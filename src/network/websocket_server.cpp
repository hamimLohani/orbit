#include "src/network/websocket_server.h"
#include "src/core/shared_data.h"
#include "src/core/app_registry.h"
#include "src/lighting/rgb_controller.h"
#include "src/lighting/indicator_led.h"
#include "src/sensors/mic_sensor.h"
#include "src/display/render_engine.h"
#include "src/core/settings_manager.h"
#include "src/network/ntp_sync.h"

WebSocketServer::WebSocketServer() : _ws("/ws") {}

void WebSocketServer::init(AsyncWebServer& server) {
    _ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
        this->onWsEvent(server, client, type, arg, data, len);
    });
    server.addHandler(&_ws);
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
        } else if (strcmp(cmd, "toast") == 0) {
            const char* title = doc["title"] | "Alert";
            const char* body = doc["body"] | "";
            uint32_t duration = doc["duration_ms"] | 3000;
            RenderEngine::getInstance().showToast(title, body, COLOR_NEON_CYAN, duration);
            IndicatorLed::getInstance().flashOnce(300);
            RgbController::getInstance().triggerFlash(0x00FFFF, 300);
        } else if (strcmp(cmd, "find_device") == 0) {
            IndicatorLed::getInstance().setPattern(LED_BLINK_FAST);
            RgbController::getInstance().triggerFlash(0xFF00FF, 1500);
        } else if (strcmp(cmd, "set_brightness") == 0) {
            uint8_t b = doc["val"] | 80;
            RenderEngine::getInstance().setBacklight(b);
            SettingsManager::getInstance().setBrightness(b);
        } else if (strcmp(cmd, "record_memo") == 0) {
            const char* action = doc["action"] | "toggle";
            if (strcmp(action, "start") == 0) {
                MicSensor::getInstance().startRecording();
            } else if (strcmp(action, "stop") == 0) {
                MicSensor::getInstance().stopRecording();
            }
        } else if (strcmp(cmd, "sync_time") == 0) {
            int tzOffset = doc["tz_offset"] | 360;
            NtpSync::getInstance().setTimezoneOffset((int16_t)tzOffset);
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
    d["moon_phase"] = round(telem.moonPhase * 100.0f) / 100.0f;
    d["recording"] = MicSensor::getInstance().isRecording();

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
