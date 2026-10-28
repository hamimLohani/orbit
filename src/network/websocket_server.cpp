#include "src/network/websocket_server.h"
#include "src/network/wifi_manager.h"
#include "src/core/shared_data.h"
#include "src/core/app_registry.h"
#include "src/input/button_manager.h"
#include "src/lighting/rgb_controller.h"
#include "src/lighting/indicator_led.h"
#include "src/sensors/mic_sensor.h"
#include "src/display/render_engine.h"
#include "src/core/settings_manager.h"
#include "src/network/ntp_sync.h"
#include "src/core/event_bus.h"

WebSocketServer::WebSocketServer() : _ws("/ws") {}

void WebSocketServer::init(AsyncWebServer& server) {
    _ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
        this->onWsEvent(server, client, type, arg, data, len);
    });
    server.addHandler(&_ws);

    // Subscribe to voice memo recording lifecycle events to stream real-time updates to website
    EventBus::getInstance().subscribe(EVENT_RECORDING_START, [this](EventType, const void* payload) {
        JsonDocument doc;
        doc["type"] = "memo_event";
        doc["event"] = "start";
        doc["mode"] = (uint8_t)MicSensor::getInstance().getMemoMode();
        doc["mode_name"] = MicSensor::getInstance().getMemoModeName();
        String out;
        serializeJson(doc, out);
        _ws.textAll(out);
    });

    EventBus::getInstance().subscribe(EVENT_RECORDING_STOP, [this](EventType, const void* payload) {
        if (!payload) return;
        const char* path = (const char*)payload;
        const char* filename = strrchr(path, '/');
        filename = filename ? (filename + 1) : path;

        JsonDocument doc;
        doc["type"] = "memo_event";
        doc["event"] = "stop";
        doc["filename"] = filename;
        doc["url"] = String("/api/download?file=") + filename;
        doc["mode"] = (uint8_t)MicSensor::getInstance().getMemoMode();
        doc["mode_name"] = MicSensor::getInstance().getMemoModeName();
        String out;
        serializeJson(doc, out);
        _ws.textAll(out);
    });
}

void WebSocketServer::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            // Send immediate telemetry on connect
            broadcastTelemetry();
            break;
        case WS_EVT_DISCONNECT:
            break;
        case WS_EVT_DATA:
            handleMessage(client, arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void WebSocketServer::handleMessage(AsyncWebSocketClient* client, void* arg, uint8_t* data, size_t len) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0; // Null-terminate
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, (char*)data);
        if (err) return;

        const char* cmd = doc["cmd"] | "";

        if (strcmp(cmd, "switch_app") == 0) {
            const char* app = doc["app"] | "home";
            AppRegistry::getInstance().switchAppById(app);
        } else if (strcmp(cmd, "set_rgb_mode") == 0) {
            uint8_t mode = doc["mode"] | 1;
            RgbController::getInstance().setMode(static_cast<RgbLightingMode>(mode));
        } else if (strcmp(cmd, "set_rgb_color") == 0) {
            const char* hexColor = doc["color"] | "#00FFFF";
            uint32_t color = (uint32_t)strtol(hexColor + (hexColor[0] == '#' ? 1 : 0), NULL, 16);
            RgbController::getInstance().setColor(color);
        } else if (strcmp(cmd, "set_rgb_brightness") == 0) {
            uint8_t b = doc["brightness"] | 180;
            RgbController::getInstance().setBrightness(b);
        } else if (strcmp(cmd, "toast") == 0) {
            const char* title = doc["title"] | "Alert";
            const char* body = doc["body"] | "";
            uint32_t duration = doc["duration_ms"] | 3000;
            RenderEngine::getInstance().showToast(title, body, COLOR_NEON_CYAN, duration);
            IndicatorLed::getInstance().flashOnce(300);
        } else if (strcmp(cmd, "find_device") == 0) {
            IndicatorLed::getInstance().setPattern(LED_BLINK_FAST);
        } else if (strcmp(cmd, "set_brightness") == 0) {
            uint8_t b = doc["val"] | 80;
            RenderEngine::getInstance().setBacklight(b);
            SettingsManager::getInstance().setBrightness(b);
        } else if (strcmp(cmd, "set_memo_mode") == 0) {
            uint8_t m = doc["mode"] | 0;
            if (m < MEMO_MODE_COUNT) {
                MicSensor::getInstance().setMemoMode(static_cast<MemoMode>(m));
            }
        } else if (strcmp(cmd, "record_memo") == 0) {
            if (doc.containsKey("mode")) {
                uint8_t m = doc["mode"] | 0;
                if (m < MEMO_MODE_COUNT) {
                    MicSensor::getInstance().setMemoMode(static_cast<MemoMode>(m));
                }
            }
            const char* action = doc["action"] | "toggle";
            if (strcmp(action, "start") == 0) {
                MicSensor::getInstance().startRecording();
            } else if (strcmp(action, "stop") == 0) {
                MicSensor::getInstance().stopRecording();
            }
        } else if (strcmp(cmd, "sync_time") == 0) {
            int tzOffset = doc["tz_offset"] | 360;
            NtpSync::getInstance().setTimezoneOffset((int16_t)tzOffset);
        } else if (strcmp(cmd, "set_focus_timer") == 0) {
            uint16_t focusMins = doc["focus_mins"] | 25;
            uint16_t breakMins = doc["break_mins"] | 5;
            SettingsManager::getInstance().setPomodoroFocusMins(focusMins);
            SettingsManager::getInstance().setPomodoroBreakMins(breakMins);
            RenderEngine::getInstance().showToast("TIMER UPDATED", "Focus Timer Configured", COLOR_NEON_MINT, 1200);
        } else if (strcmp(cmd, "sync_weather") == 0) {
            float temp = doc["temp"] | 21.0f;
            float feels = doc["feels_like"] | temp;
            float hum = doc["hum"] | 50.0f;
            float wind = doc["wind"] | 3.0f;
            uint16_t press = doc["pressure"] | 1013;
            const char* cond = doc["condition"] | "Clear";
            const char* city = doc["city"] | "Local";
            const char* desc = doc["desc"] | "Clear sky";
            SharedData::getInstance().setWeather(temp, feels, hum, wind, press, cond, city, desc);
            RenderEngine::getInstance().showToast("WEATHER SYNC", city, COLOR_NEON_AMBER, 1200);
        } else if (strcmp(cmd, "show_voice_text") == 0) {
            const char* title = doc["title"] | "TRANSCRIPT";
            const char* sub = doc["subtitle"] | "VOICE MEMO";
            const char* text = doc["text"] | doc["transcript"] | "";
            RenderEngine::getInstance().showTextOverlay(title, sub, text);
            IndicatorLed::getInstance().flashOnce(250);
        } else if (strcmp(cmd, "show_ai_answer") == 0) {
            const char* q = doc["question"] | "AI Question";
            const char* a = doc["answer"] | "No answer provided.";
            const char* title = doc["title"] | "AI ANSWER";
            RenderEngine::getInstance().showTextOverlay(title, q, a);
        } else if (strcmp(cmd, "hide_ai_answer") == 0 || strcmp(cmd, "hide_voice_text") == 0) {
            RenderEngine::getInstance().hideAiAnswer();
        } else if (strcmp(cmd, "button_press") == 0) {
            const char* btnStr = doc["button"] | "enter";
            const char* typeStr = doc["type"] | "click";
            ButtonId btn = BTN_ID_ENTER;
            if (strcmp(btnStr, "up") == 0) btn = BTN_ID_UP;
            else if (strcmp(btnStr, "down") == 0) btn = BTN_ID_DOWN;
            else if (strcmp(btnStr, "left") == 0) btn = BTN_ID_LEFT;
            else if (strcmp(btnStr, "right") == 0) btn = BTN_ID_RIGHT;
            else if (strcmp(btnStr, "enter") == 0 || strcmp(btnStr, "ok") == 0) btn = BTN_ID_ENTER;
            else if (strcmp(btnStr, "back") == 0) btn = BTN_ID_ENTER;
            else if (strcmp(btnStr, "home") == 0) btn = BTN_ID_ENTER;

            ButtonEventType bType = BUTTON_CLICK;
            if (strcmp(typeStr, "hold") == 0 || strcmp(btnStr, "back") == 0) {
                bType = BUTTON_HOLD_1S;
            } else if (strcmp(typeStr, "home") == 0 || strcmp(btnStr, "home") == 0) {
                bType = BUTTON_HOLD_3S;
            }

            ButtonEvent evt = { btn, bType, 100 };
            AppRegistry::getInstance().handleButtonEvent(evt);
        } else if (strcmp(cmd, "forget_wifi") == 0) {
            WifiManager::getInstance().forgetWifi();
        }
    }
}

void WebSocketServer::broadcastTelemetry() {
    if (_ws.count() == 0) return;

    TelemetryData telem = SharedData::getInstance().getData();
    JsonDocument doc;
    doc["type"] = "telemetry";

    JsonObject d = doc["d"].to<JsonObject>();
    d["temp"] = round(telem.temperature * 10.0f) / 10.0f;
    d["hum"] = round(telem.humidity * 10.0f) / 10.0f;
    d["heat_idx"] = round(telem.heatIndex * 10.0f) / 10.0f;
    d["mic_db"] = round(telem.micDb * 10.0f) / 10.0f;
    d["wifi_rssi"] = telem.wifiRssi;
    d["ip"] = telem.ipAddress;
    d["time"] = telem.timeStr;
    d["date"] = telem.dateStr;
    d["time_synced"] = telem.timeSynced;
    d["active_app"] = telem.activeAppId;
    d["heap"] = telem.freeHeap;
    d["uptime"] = telem.uptimeSec;
    d["rgb_mode"] = telem.rgbMode;
    d["rgb_color"] = telem.rgbColor;
    d["rgb_brightness"] = RgbController::getInstance().getBrightness();
    d["brightness"] = telem.brightness;
    d["moon_phase"] = round(telem.moonPhase * 100.0f) / 100.0f;
    d["recording"] = MicSensor::getInstance().isRecording();
    d["memo_mode"] = (uint8_t)MicSensor::getInstance().getMemoMode();
    d["memo_mode_name"] = MicSensor::getInstance().getMemoModeName();

    // Outdoor Weather
    d["weather_valid"] = telem.weatherValid;
    d["outdoor_temp"] = round(telem.outdoorTemp * 10.0f) / 10.0f;
    d["outdoor_feels"] = round(telem.outdoorFeelsLike * 10.0f) / 10.0f;
    d["outdoor_hum"] = round(telem.outdoorHumidity * 10.0f) / 10.0f;
    d["wind_speed"] = round(telem.windSpeedMps * 10.0f) / 10.0f;
    d["pressure_hpa"] = telem.pressureHpa;
    d["weather_city"] = telem.weatherCity;
    d["weather_cond"] = telem.weatherCondition;
    d["weather_desc"] = telem.weatherDesc;

    JsonArray bins = d["fft"].to<JsonArray>();
    for (int i = 0; i < FFT_NUM_BINS; ++i) {
        bins.add(telem.fftBands[i]);
    }

    String output;
    serializeJson(doc, output);
    _ws.textAll(output);
}

void WebSocketServer::sendToAll(const String& message) {
    if (_ws.count() > 0) {
        _ws.textAll(message);
    }
}

void WebSocketServer::update() {
    uint32_t now = millis();
    if (now - _lastBroadcast >= TELEMETRY_BROADCAST_MS) {
        _lastBroadcast = now;
        broadcastTelemetry();
    }
    _ws.cleanupClients();
}
