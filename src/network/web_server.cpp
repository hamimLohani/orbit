#include "src/network/web_server.h"
#include "src/network/websocket_server.h"
#include "src/network/wifi_manager.h"
#include "src/network/wifi_setup_page.h"
#include "src/network/embedded_web_assets.h"
#include "src/network/ota_web_page.h"
#include "src/network/ota_handler.h"
#include "src/core/shared_data.h"
#include "src/core/app_registry.h"
#include "src/core/settings_manager.h"
#include "src/display/bitmap_loader.h"
#include "src/display/render_engine.h"
#include <ArduinoJson.h>
#include <Update.h>
#include <esp_system.h>
#include <esp_idf_version.h>

static void sendDashboardHtml(AsyncWebServerRequest* request) {
    if (LittleFS.exists("/web/index.html")) {
        request->send(LittleFS, "/web/index.html", "text/html");
        return;
    }
    SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
    bool sdExists = false;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        digitalWrite(PIN_TFT_CS, HIGH);
        sdExists = SD.exists("/web/index.html");
        xSemaphoreGive(mutex);
    }
    if (sdExists) {
        request->send(SD, "/web/index.html", "text/html");
        return;
    }
    AsyncWebServerResponse* res = request->beginResponse_P(200, "text/html", EMBEDDED_INDEX_HTML_GZ, EMBEDDED_INDEX_HTML_GZ_LEN);
    res->addHeader("Content-Encoding", "gzip");
    res->addHeader("Cache-Control", "no-cache");
    request->send(res);
}

static String processWifiSetupHtml(const String& var) {
    if (var == "NETWORKS_LIST") {
        const auto& networks = WifiManager::getInstance().getScannedNetworks();
        if (networks.empty()) {
            return "<div class=\"empty-state\"><div class=\"spinner\" style=\"margin: 0 auto 12px; width: 24px; height: 24px; border: 2px solid var(--border); border-top-color: var(--cyan); border-radius: 50%; animation: spin 1s linear infinite;\"></div>Scanning for nearby networks...</div>";
        }
        String html = "";
        for (const auto& net : networks) {
            String lockSvg = net.secure ? "<svg width=\"12\" height=\"12\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"var(--muted)\" stroke-width=\"2.5\"><rect x=\"3\" y=\"11\" width=\"18\" height=\"11\" rx=\"2\" ry=\"2\"></rect><path d=\"M7 11V7a5 5 0 0 1 10 0v4\"></path></svg>" : "";
            int bars = net.rssi > -60 ? 4 : (net.rssi > -70 ? 3 : (net.rssi > -80 ? 2 : 1));
            String barColor = net.rssi > -65 ? "var(--mint)" : (net.rssi > -78 ? "var(--cyan)" : "var(--red)");
            String sigSvg = "<svg width=\"15\" height=\"15\" viewBox=\"0 0 24 24\" fill=\"" + barColor + "\">";
            sigSvg += "<rect x=\"2\" y=\"16\" width=\"3\" height=\"6\" rx=\"1\" opacity=\"" + String(bars >= 1 ? "1" : "0.25") + "\"/>";
            sigSvg += "<rect x=\"7\" y=\"12\" width=\"3\" height=\"10\" rx=\"1\" opacity=\"" + String(bars >= 2 ? "1" : "0.25") + "\"/>";
            sigSvg += "<rect x=\"12\" y=\"8\" width=\"3\" height=\"14\" rx=\"1\" opacity=\"" + String(bars >= 3 ? "1" : "0.25") + "\"/>";
            sigSvg += "<rect x=\"17\" y=\"4\" width=\"3\" height=\"18\" rx=\"1\" opacity=\"" + String(bars >= 4 ? "1" : "0.25") + "\"/>";
            sigSvg += "</svg>";

            html += "<div class=\"net-item\" onclick=\"selectNetwork(this, '" + net.ssid + "')\">";
            html += "<div class=\"net-name\">" + lockSvg + "<span>" + net.ssid + "</span></div>";
            html += "<div class=\"net-meta\"><span class=\"net-rssi\">" + String(net.rssi) + " dBm</span>" + sigSvg + "</div>";
            html += "</div>";
        }
        return html;
    }
    return String();
}

static void sendStyleCss(AsyncWebServerRequest* request) {
    if (LittleFS.exists("/web/style.css")) {
        request->send(LittleFS, "/web/style.css", "text/css");
        return;
    }
    SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
    bool sdExists = false;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        digitalWrite(PIN_TFT_CS, HIGH);
        sdExists = SD.exists("/web/style.css");
        xSemaphoreGive(mutex);
    }
    if (sdExists) {
        request->send(SD, "/web/style.css", "text/css");
        return;
    }
    AsyncWebServerResponse* res = request->beginResponse_P(200, "text/css", EMBEDDED_STYLE_CSS_GZ, EMBEDDED_STYLE_CSS_GZ_LEN);
    res->addHeader("Content-Encoding", "gzip");
    res->addHeader("Cache-Control", "public, max-age=86400");
    request->send(res);
}

static void sendAppJs(AsyncWebServerRequest* request) {
    if (LittleFS.exists("/web/app.js")) {
        request->send(LittleFS, "/web/app.js", "application/javascript");
        return;
    }
    SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
    bool sdExists = false;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        digitalWrite(PIN_TFT_CS, HIGH);
        sdExists = SD.exists("/web/app.js");
        xSemaphoreGive(mutex);
    }
    if (sdExists) {
        request->send(SD, "/web/app.js", "application/javascript");
        return;
    }
    AsyncWebServerResponse* res = request->beginResponse_P(200, "application/javascript", EMBEDDED_APP_JS_GZ, EMBEDDED_APP_JS_GZ_LEN);
    res->addHeader("Content-Encoding", "gzip");
    res->addHeader("Cache-Control", "public, max-age=86400");
    request->send(res);
}

WebServer::WebServer() : _server(HTTP_PORT) {}

void WebServer::init() {
    _fsMounted = LittleFS.begin(false); // Do not block with auto-formatting on boot

    // Attach WebSocket
    WebSocketServer::getInstance().init(_server);

    setupRoutes();
    setupRestApi();

    _server.begin();
    Serial.println("[WebServer] HTTP & Captive Portal server started on port 80.");
}

void WebServer::setupRoutes() {
    // 1. Wi-Fi Provisioning & Captive Portal Setup Page
    _server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", WIFI_SETUP_HTML, processWifiSetupHtml);
    });
    _server.on("/setup", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/wifi");
    });

    // 2. OS Captive Portal Probe Endpoints (Apple iOS/macOS, Android, Windows)
    _server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/wifi");
    });
    _server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/wifi");
    });
    _server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/wifi");
    });
    _server.on("/canonical.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/wifi");
    });
    _server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", "Microsoft NCSI");
    });
    _server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/wifi");
    });
    _server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/wifi");
    });

    // 3. Web OTA Firmware & Filesystem Update Studio (GET /update)
    _server.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", OTA_UPDATE_HTML);
    });

    // 4. Web OTA Binary Upload (POST /update)
    _server.on("/update", HTTP_POST, [](AsyncWebServerRequest* request) {
        bool shouldReboot = !Update.hasError();
        AsyncWebServerResponse* response = request->beginResponse(200, "application/json",
            shouldReboot ? "{\"status\":\"success\",\"message\":\"OTA Flash Complete! Rebooting...\"}"
                         : "{\"status\":\"error\",\"message\":\"Flash error occurred.\"}");
        response->addHeader("Connection", "close");
        request->send(response);
        if (shouldReboot) {
            RenderEngine::getInstance().showToast("OTA SUCCESS", "Rebooting system...", COLOR_NEON_MINT, 3000);
            delay(600);
            esp_restart();
        }
    }, [](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
        if (!index) {
            Serial.printf("[WebOTA] Starting upload: %s\n", filename.c_str());
            String target = request->hasParam("target") ? request->getParam("target")->value() : "flash";
            int cmd = (target == "fs" || filename.indexOf("littlefs") >= 0 || filename.indexOf("spiffs") >= 0) ? U_SPIFFS : U_FLASH;
            if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
                Update.printError(Serial);
                OtaHandler::getInstance().setUpdating(false, 0, "Init Error");
            } else {
                OtaHandler::getInstance().setUpdating(true, 0, "Flashing...");
                RenderEngine::getInstance().showToast("WEB OTA", "Receiving binary...", COLOR_NEON_CYAN, 5000);
            }
        }
        if (!Update.hasError()) {
            if (Update.write(data, len) != len) {
                Update.printError(Serial);
                OtaHandler::getInstance().setUpdating(false, 0, "Write Error");
            } else {
                size_t current = index + len;
                size_t total = request->contentLength();
                uint8_t pct = (total > 0) ? (uint8_t)((current * 100) / total) : 50;
                OtaHandler::getInstance().setUpdating(true, pct, "Flashing...");
            }
        }
        if (final) {
            if (Update.end(true)) {
                Serial.printf("[WebOTA] Success: %u bytes\n", index + len);
                OtaHandler::getInstance().setUpdating(false, 100, "Done! Rebooting");
            } else {
                Update.printError(Serial);
                OtaHandler::getInstance().setUpdating(false, 0, "End Error");
            }
        }
    });

    // 3. Static Web Companion Assets (LittleFS -> SD Card -> Embedded PROGMEM GZIP)
    _server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        sendStyleCss(request);
    });
    _server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        sendAppJs(request);
    });
    _server.on("/dash", HTTP_GET, [](AsyncWebServerRequest* request) {
        sendDashboardHtml(request);
    });
    _server.on("/app", HTTP_GET, [](AsyncWebServerRequest* request) {
        sendDashboardHtml(request);
    });
    _server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        sendDashboardHtml(request);
    });

    if (_fsMounted) {
        _server.serveStatic("/", LittleFS, "/web/");
    }

    // 4. Root / Landing Page Handler
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        // Query param override: ?setup forces Wi-Fi setup page
        if (request->hasParam("setup")) {
            request->send_P(200, "text/html", WIFI_SETUP_HTML, processWifiSetupHtml);
            return;
        }

        // Query param override: ?dash or ?app forces full companion dashboard
        if (request->hasParam("dash") || request->hasParam("app")) {
            sendDashboardHtml(request);
            return;
        }

        // If connected to home Wi-Fi station, always serve dashboard
        if (WifiManager::getInstance().isConnected()) {
            sendDashboardHtml(request);
            return;
        }

        // If in SoftAP mode and not connected to home Wi-Fi, serve setup portal
        // (which features a 1-click button to launch the dashboard directly in SoftAP mode)
        if (WifiManager::getInstance().isApActive()) {
            request->send_P(200, "text/html", WIFI_SETUP_HTML, processWifiSetupHtml);
            return;
        }

        sendDashboardHtml(request);
    });
}

void WebServer::setupRestApi() {
    // 0. API Root Catalog & Documentation (GET /api)
    _server.on("/api", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["name"] = "ORBIT v2 Ambient Companion OS";
        doc["version"] = ORBIT_VERSION_STR;
        JsonObject endpoints = doc["endpoints"].to<JsonObject>();
        endpoints["telemetry"]    = "GET /api/telemetry";
        endpoints["apps"]         = "GET /api/apps";
        endpoints["recordings"]   = "GET /api/recordings";
        endpoints["download"]     = "GET /api/download?file=<name>";
        endpoints["delete_memo"]  = "POST /api/recordings/delete?file=<name>";
        endpoints["wifi_scan"]    = "GET /api/wifi/scan";
        endpoints["wifi_status"]  = "GET /api/wifi/status";
        endpoints["wifi_connect"] = "POST /api/wifi/connect";
        endpoints["wifi_forget"]  = "POST /api/wifi/forget";
        endpoints["system_info"]  = "GET /api/system/info";
        endpoints["system_reboot"]= "POST /api/system/reboot";
        endpoints["files_upload"] = "POST /api/files/upload?target=[sd|lfs]&path=<path>";
        endpoints["ota_status"]   = "GET /api/ota/status";
        endpoints["ota_update"]      = "POST /update";
        endpoints["ai_answer"]       = "POST /api/ai/answer";
        endpoints["ai_clear"]        = "POST /api/ai/clear";
        endpoints["keys"]            = "GET /api/keys";
        endpoints["supabase_config"] = "GET /api/supabase/config";
        endpoints["supabase_sync"]   = "POST /api/supabase/sync?file=<name>";
        endpoints["firebase_config"] = "GET /api/firebase/config";
        endpoints["firebase_sync"]   = "POST /api/firebase/sync?file=<name>";

        String res;
        serializeJson(doc, res);
        request->send(200, "application/json", res);
    });

    // 0b. Cloud Keys & Service Provisioning API (GET /api/keys)
    _server.on("/api/keys", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["openai_configured"]      = (strlen(DEFAULT_OPENAI_API_KEY) > 0);
        doc["gemini_configured"]      = (strlen(DEFAULT_GEMINI_API_KEY) > 0);
        doc["elevenlabs_configured"]  = (strlen(DEFAULT_ELEVENLABS_API_KEY) > 0);
        doc["weather_configured"]     = (strlen(DEFAULT_OPENWEATHER_API_KEY) > 0);
        doc["supabase_configured"]    = (strlen(DEFAULT_SUPABASE_URL) > 0 && strlen(DEFAULT_SUPABASE_ANON_KEY) > 0);
        doc["firebase_configured"]    = (strlen(DEFAULT_FIREBASE_PROJECT_ID) > 0 && strlen(DEFAULT_FIREBASE_API_KEY) > 0);

        // Expose secrets configured at compile-time to local companion
        doc["openai_key"]             = DEFAULT_OPENAI_API_KEY;
        doc["gemini_key"]             = DEFAULT_GEMINI_API_KEY;
        doc["elevenlabs_key"]         = DEFAULT_ELEVENLABS_API_KEY;
        doc["weather_key"]            = DEFAULT_OPENWEATHER_API_KEY;
        doc["weather_city"]           = DEFAULT_WEATHER_CITY;
        doc["default_ai_engine"]      = ACTIVE_AI_ENGINE;

        // Supabase Cloud Database & Storage Credentials (100% Free Tier, No CC)
        doc["supabase_url"]           = DEFAULT_SUPABASE_URL;
        doc["supabase_anon_key"]      = DEFAULT_SUPABASE_ANON_KEY;
        doc["supabase_storage_bucket"]= DEFAULT_SUPABASE_STORAGE_BUCKET;
        doc["supabase_table"]         = DEFAULT_SUPABASE_TABLE;

        // Firebase Cloud Database & Storage Credentials
        doc["firebase_project_id"]    = DEFAULT_FIREBASE_PROJECT_ID;
        doc["firebase_api_key"]       = DEFAULT_FIREBASE_API_KEY;
        doc["firebase_storage_bucket"]= DEFAULT_FIREBASE_STORAGE_BUCKET;
        doc["firebase_collection"]    = DEFAULT_FIREBASE_COLLECTION;
        doc["firebase_rtdb_url"]      = DEFAULT_FIREBASE_RTDB_URL;

        String res;
        serializeJson(doc, res);
        request->send(200, "application/json", res);
    });

    // 0c. Supabase Configuration API (GET /api/supabase/config)
    _server.on("/api/supabase/config", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["configured"]     = (strlen(DEFAULT_SUPABASE_URL) > 0 && strlen(DEFAULT_SUPABASE_ANON_KEY) > 0);
        doc["url"]            = DEFAULT_SUPABASE_URL;
        doc["anon_key"]       = DEFAULT_SUPABASE_ANON_KEY;
        doc["storage_bucket"] = DEFAULT_SUPABASE_STORAGE_BUCKET;
        doc["table"]          = DEFAULT_SUPABASE_TABLE;

        String res;
        serializeJson(doc, res);
        request->send(200, "application/json", res);
    });

    // 0d. Prepare Recording for Supabase Direct Upload (POST /api/supabase/sync)
    _server.on("/api/supabase/sync", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!request->hasParam("file", true) && !request->hasParam("file")) {
            request->send(400, "application/json", "{\"error\":\"Missing file parameter\"}");
            return;
        }
        String filename = request->hasParam("file", true) ? request->getParam("file", true)->value() : request->getParam("file")->value();
        String fullPath = String(SD_PATH_MEMOS) + "/" + filename;

        bool exists = false;
        size_t fileSize = 0;
        SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            digitalWrite(PIN_TFT_CS, HIGH);
            if (SD.exists(fullPath.c_str())) {
                File f = SD.open(fullPath.c_str(), FILE_READ);
                if (f) {
                    fileSize = f.size();
                    f.close();
                    exists = true;
                }
            }
            xSemaphoreGive(mutex);
        }

        if (!exists) {
            request->send(404, "application/json", "{\"error\":\"File not found on SD card\"}");
            return;
        }

        TelemetryData telem = SharedData::getInstance().getData();
        JsonDocument doc;
        doc["status"]       = "ready";
        doc["filename"]     = filename;
        doc["size_bytes"]   = fileSize;
        doc["download_url"] = "/api/download?file=" + filename;
        doc["temp_c"]       = telem.temperature;
        doc["humidity_pct"] = telem.humidity;
        doc["noise_db"]     = telem.micDb;
        doc["timestamp_ms"] = millis();

        String res;
        serializeJson(doc, res);
        request->send(200, "application/json", res);
    });

    // 0c. Firebase Configuration API (GET /api/firebase/config)
    _server.on("/api/firebase/config", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["configured"]     = (strlen(DEFAULT_FIREBASE_PROJECT_ID) > 0 && strlen(DEFAULT_FIREBASE_API_KEY) > 0);
        doc["project_id"]     = DEFAULT_FIREBASE_PROJECT_ID;
        doc["api_key"]        = DEFAULT_FIREBASE_API_KEY;
        doc["storage_bucket"] = DEFAULT_FIREBASE_STORAGE_BUCKET;
        doc["collection"]     = DEFAULT_FIREBASE_COLLECTION;
        doc["rtdb_url"]       = DEFAULT_FIREBASE_RTDB_URL;

        String res;
        serializeJson(doc, res);
        request->send(200, "application/json", res);
    });

    // 0d. Prepare Recording for Firebase Database Direct Upload (POST /api/firebase/sync)
    _server.on("/api/firebase/sync", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!request->hasParam("file", true) && !request->hasParam("file")) {
            request->send(400, "application/json", "{\"error\":\"Missing file parameter\"}");
            return;
        }
        String filename = request->hasParam("file", true) ? request->getParam("file", true)->value() : request->getParam("file")->value();
        String fullPath = String(SD_PATH_MEMOS) + "/" + filename;

        bool exists = false;
        size_t fileSize = 0;
        SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            digitalWrite(PIN_TFT_CS, HIGH);
            if (SD.exists(fullPath.c_str())) {
                File f = SD.open(fullPath.c_str(), FILE_READ);
                if (f) {
                    exists = true;
                    fileSize = f.size();
                    f.close();
                }
            }
            xSemaphoreGive(mutex);
        }

        if (!exists) {
            request->send(404, "application/json", "{\"error\":\"Recording file not found on SD card\"}");
            return;
        }

        TelemetryData telem = SharedData::getInstance().getData();
        JsonDocument doc;
        doc["status"] = "ready_for_sync";
        doc["filename"] = filename;
        doc["file_size_bytes"] = fileSize;
        doc["audio_download_url"] = String("/api/download?file=") + filename;
        doc["duration_sec"] = (fileSize > 44) ? ((fileSize - 44) / 32000) : 0; // 16kHz 16-bit mono = 32000 B/s
        doc["device_id"] = "ORBIT-ESP32-S3";
        doc["temperature_c"] = telem.temperature;
        doc["humidity_pct"] = telem.humidity;
        doc["noise_db"] = telem.micDb;
        doc["timestamp"] = telem.timeStr;
        doc["date"] = telem.dateStr;

        String res;
        serializeJson(doc, res);
        request->send(200, "application/json", res);
    });

    // 1. Telemetry API
    _server.on("/api/telemetry", HTTP_GET, [](AsyncWebServerRequest* request) {
        TelemetryData telem = SharedData::getInstance().getData();
        JsonDocument doc;
        doc["temp"] = telem.temperature;
        doc["hum"] = telem.humidity;
        doc["mic_db"] = telem.micDb;
        doc["wifi_rssi"] = telem.wifiRssi;
        doc["uptime"] = telem.uptimeSec;
        doc["heap"] = telem.freeHeap;
        doc["active_app"] = telem.activeAppId;

        String res;
        serializeJson(doc, res);
        request->send(200, "application/json", res);
    });

    // 1b. Live Weather API (GET /api/weather, POST /api/weather)
    _server.on("/api/weather", HTTP_GET, [](AsyncWebServerRequest* request) {
        TelemetryData telem = SharedData::getInstance().getData();
        JsonDocument doc;
        doc["valid"]        = telem.weatherValid;
        doc["outdoor_temp"] = telem.outdoorTemp;
        doc["feels_like"]   = telem.outdoorFeelsLike;
        doc["outdoor_hum"]  = telem.outdoorHumidity;
        doc["wind_speed"]   = telem.windSpeedMps;
        doc["pressure_hpa"] = telem.pressureHpa;
        doc["condition"]    = telem.weatherCondition;
        doc["city"]         = telem.weatherCity;
        doc["description"]  = telem.weatherDesc;
        doc["indoor_temp"]  = telem.temperature;
        doc["indoor_hum"]   = telem.humidity;
        doc["delta_temp"]   = round((telem.temperature - telem.outdoorTemp) * 10.0f) / 10.0f;
        doc["delta_hum"]    = round((telem.humidity - telem.outdoorHumidity) * 10.0f) / 10.0f;

        String res;
        serializeJson(doc, res);
        request->send(200, "application/json", res);
    });

    // 2. Apps Registry API
    _server.on("/api/apps", HTTP_GET, [](AsyncWebServerRequest* request) {
        const auto& apps = AppRegistry::getInstance().getApps();
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();

        for (const auto* app : apps) {
            AppMetadata meta = app->getMetadata();
            JsonObject item = arr.add<JsonObject>();
            item["id"] = meta.id;
            item["title"] = meta.title;
            item["icon"] = meta.iconIndex;
        }

        String res;
        serializeJson(doc, res);
        request->send(200, "application/json", res);
    });

    // 3. SD Voice Memos List
    _server.on("/api/recordings", HTTP_GET, [](AsyncWebServerRequest* request) {
        std::vector<String> files = BitmapLoader::getInstance().listFiles(SD_PATH_MEMOS);
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (const auto& f : files) {
            arr.add(f);
        }
        String res;
        serializeJson(doc, res);
        request->send(200, "application/json", res);
    });

    // 4. Download Recording Audio (.wav)
    _server.on("/api/download", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!request->hasParam("file")) {
            request->send(400, "text/plain", "Missing file param");
            return;
        }
        String filename = request->getParam("file")->value();
        String fullPath = String(SD_PATH_MEMOS) + "/" + filename;

        SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            digitalWrite(PIN_TFT_CS, HIGH);
            if (SD.exists(fullPath.c_str())) {
                xSemaphoreGive(mutex);
                request->send(SD, fullPath, "audio/wav");
                return;
            }
            xSemaphoreGive(mutex);
        }
        request->send(404, "text/plain", "File not found");
    });

    // 4b. Delete Recording Audio (.wav)
    _server.on("/api/recordings/delete", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (!request->hasParam("file", true) && !request->hasParam("file")) {
            request->send(400, "application/json", "{\"error\":\"Missing file parameter\"}");
            return;
        }
        String filename = request->hasParam("file", true) ? request->getParam("file", true)->value() : request->getParam("file")->value();
        String fullPath = String(SD_PATH_MEMOS) + "/" + filename;

        bool deleted = false;
        SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            digitalWrite(PIN_TFT_CS, HIGH);
            if (SD.exists(fullPath.c_str())) {
                deleted = SD.remove(fullPath.c_str());
            }
            xSemaphoreGive(mutex);
        }

        if (deleted) {
            request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Recording deleted\"}");
        } else {
            request->send(404, "application/json", "{\"error\":\"File not found or failed to delete\"}");
        }
    });

    // 5. Wi-Fi Scan Results API (GET /api/wifi/scan)
    _server.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!WifiManager::getInstance().isScanning() && WifiManager::getInstance().getScannedNetworks().empty()) {
            WifiManager::getInstance().requestScan();
        }
        String res = WifiManager::getInstance().getScannedNetworksJson();
        request->send(200, "application/json", res);
    });

    // 6. Trigger New Wi-Fi Scan (POST /api/wifi/scan)
    _server.on("/api/wifi/scan", HTTP_POST, [](AsyncWebServerRequest* request) {
        WifiManager::getInstance().requestScan();
        request->send(200, "application/json", "{\"status\":\"scanning\"}");
    });

    // 7. Wi-Fi Status API (GET /api/wifi/status)
    _server.on("/api/wifi/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["connected"] = WifiManager::getInstance().isConnected();
        doc["connecting"] = WifiManager::getInstance().isConnecting();
        doc["ap_active"] = WifiManager::getInstance().isApActive();
        doc["ssid"] = WifiManager::getInstance().getSsid();
        doc["ip"] = WifiManager::getInstance().getIpAddress();
        doc["rssi"] = WifiManager::getInstance().getRssi();
        doc["softap_ip"] = WifiManager::getInstance().getSoftApIp();
        doc["softap_ssid"] = WifiManager::getInstance().getSoftApSsid();

        String res;
        serializeJson(doc, res);
        request->send(200, "application/json", res);
    });

    // 8. Connect to Wi-Fi (POST /api/wifi/connect with form data or URL params)
    _server.on("/api/wifi/connect", HTTP_POST, [](AsyncWebServerRequest* request) {
        String ssid = "";
        String pass = "";

        if (request->hasParam("ssid", true)) {
            ssid = request->getParam("ssid", true)->value();
        } else if (request->hasParam("ssid")) {
            ssid = request->getParam("ssid")->value();
        }

        if (request->hasParam("pass", true)) {
            pass = request->getParam("pass", true)->value();
        } else if (request->hasParam("pass")) {
            pass = request->getParam("pass")->value();
        }

        ssid.trim();

        if (ssid.length() == 0) {
            request->send(400, "application/json", "{\"error\":\"Missing SSID parameter\"}");
            return;
        }

        WifiManager::getInstance().connect(ssid, pass);

        JsonDocument res;
        res["status"] = "connecting";
        res["ssid"] = ssid;
        res["message"] = "Initiating connection to " + ssid;
        String out;
        serializeJson(res, out);
        request->send(200, "application/json", out);
    });

    // 9. Connect to Wi-Fi fallback (GET /api/wifi/connect?ssid=...&pass=...)
    _server.on("/api/wifi/connect", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!request->hasParam("ssid")) {
            request->send(400, "application/json", "{\"error\":\"Missing SSID parameter\"}");
            return;
        }
        String ssid = request->getParam("ssid")->value();
        String pass = request->hasParam("pass") ? request->getParam("pass")->value() : "";
        ssid.trim();

        WifiManager::getInstance().connect(ssid, pass);

        JsonDocument res;
        res["status"] = "connecting";
        res["ssid"] = ssid;
        res["message"] = "Initiating connection to " + ssid;
        String out;
        serializeJson(res, out);
        request->send(200, "application/json", out);
    });

    // 9b. Forget Saved Wi-Fi Credentials API (POST /api/wifi/forget & GET /api/wifi/forget)
    _server.on("/api/wifi/forget", HTTP_POST, [](AsyncWebServerRequest* request) {
        WifiManager::getInstance().forgetWifi();
        request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Wi-Fi credentials forgotten. Switched to SoftAP mode.\"}");
    });
    _server.on("/api/wifi/forget", HTTP_GET, [](AsyncWebServerRequest* request) {
        WifiManager::getInstance().forgetWifi();
        request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Wi-Fi credentials forgotten. Switched to SoftAP mode.\"}");
    });

    // 10. Comprehensive Hardware & OS Diagnostics API (GET /api/system/info)
    _server.on("/api/system/info", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;

        // Device & Silicon
        JsonObject dev = doc["device"].to<JsonObject>();
        dev["chip"] = ESP.getChipModel();
        dev["revision"] = ESP.getChipRevision();
        dev["cores"] = ESP.getChipCores();
        dev["cpu_mhz"] = ESP.getCpuFreqMHz();
        dev["mac"] = WiFi.macAddress();
        uint64_t efuse = ESP.getEfuseMac();
        char efuseStr[24];
        snprintf(efuseStr, sizeof(efuseStr), "0x%08llX", efuse & 0xFFFFFFFFULL);
        dev["uid"] = efuseStr;

        // OS & Firmware
        JsonObject os = doc["os"].to<JsonObject>();
        os["name"] = "ORBIT OS";
        os["version"] = ORBIT_VERSION_STR;
        os["idf"] = esp_get_idf_version();
        os["kernel"] = "FreeRTOS 10";
        os["uptime_sec"] = millis() / 1000;

        // Storage: SD & LittleFS
        JsonObject storage = doc["storage"].to<JsonObject>();
        bool sdMounted = BitmapLoader::getInstance().isSdMounted();
        storage["sd_mounted"] = sdMounted;
        if (sdMounted) {
            uint32_t totalMb = 0, usedMb = 0;
            BitmapLoader::getInstance().getStorageStats(totalMb, usedMb);
            storage["sd_total_mb"] = totalMb;
            storage["sd_used_mb"] = usedMb;
            storage["sd_free_mb"] = (totalMb >= usedMb) ? (totalMb - usedMb) : 0;
        } else {
            storage["sd_total_mb"] = 0;
            storage["sd_used_mb"] = 0;
            storage["sd_free_mb"] = 0;
        }
        storage["lfs_total_bytes"] = LittleFS.totalBytes();
        storage["lfs_used_bytes"] = LittleFS.usedBytes();

        // Flash & ROM
        JsonObject flash = doc["flash"].to<JsonObject>();
        flash["size_bytes"] = ESP.getFlashChipSize();
        flash["speed_hz"] = ESP.getFlashChipSpeed();
        flash["sketch_bytes"] = ESP.getSketchSize();
        flash["free_sketch_bytes"] = ESP.getFreeSketchSpace();

        // Memory Architecture
        JsonObject mem = doc["memory"].to<JsonObject>();
        mem["heap_total"] = ESP.getHeapSize();
        mem["heap_free"] = ESP.getFreeHeap();
        mem["heap_min_free"] = ESP.getMinFreeHeap();
        mem["psram_found"] = psramFound();
        mem["psram_total"] = psramFound() ? ESP.getPsramSize() : 0;
        mem["psram_free"] = psramFound() ? ESP.getFreePsram() : 0;

        // Network
        JsonObject net = doc["network"].to<JsonObject>();
        net["connected"] = WifiManager::getInstance().isConnected();
        net["ssid"] = WifiManager::getInstance().getSsid();
        net["ip"] = WifiManager::getInstance().getIpAddress();
        net["gateway"] = WiFi.gatewayIP().toString();
        net["subnet"] = WiFi.subnetMask().toString();
        net["rssi"] = WifiManager::getInstance().getRssi();
        net["ap_active"] = WifiManager::getInstance().isApActive();
        net["softap_ssid"] = WifiManager::getInstance().getSoftApSsid();
        net["softap_ip"] = WifiManager::getInstance().getSoftApIp();
        net["mdns"] = String(MDNS_HOSTNAME) + ".local";

        String res;
        serializeJson(doc, res);
        request->send(200, "application/json", res);
    });

    // 11. System Reboot API (POST /api/system/reboot)
    _server.on("/api/system/reboot", HTTP_POST, [](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"status\":\"rebooting\"}");
        delay(200);
        esp_restart();
    });

    // 12. Wi-Fi File Upload to SD / LittleFS (POST /api/files/upload)
    _server.on("/api/files/upload", HTTP_POST, [](AsyncWebServerRequest* request) {
        request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"File uploaded successfully\"}");
    }, [](AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
        static File uploadFile;
        static bool isSd = true;
        if (!index) {
            String target = request->hasParam("target") ? request->getParam("target")->value() : "sd";
            String path = request->hasParam("path") ? request->getParam("path")->value() : "";
            isSd = (target != "lfs");

            if (isSd) {
                SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
                if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    digitalWrite(PIN_TFT_CS, HIGH);
                    String fullPath = path.isEmpty() ? ("/" + filename) : (path + "/" + filename);
                    uploadFile = SD.open(fullPath.c_str(), FILE_WRITE);
                    xSemaphoreGive(mutex);
                }
            } else {
                String fullPath = path.isEmpty() ? ("/" + filename) : (path + "/" + filename);
                uploadFile = LittleFS.open(fullPath.c_str(), "w");
            }
            Serial.printf("[Upload] Starting Wi-Fi file upload: %s (target: %s)\n", filename.c_str(), target.c_str());
        }
        if (uploadFile) {
            if (isSd) {
                SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
                if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    digitalWrite(PIN_TFT_CS, HIGH);
                    uploadFile.write(data, len);
                    xSemaphoreGive(mutex);
                }
            } else {
                uploadFile.write(data, len);
            }
        }
        if (final) {
            if (uploadFile) {
                if (isSd) {
                    SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
                    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        digitalWrite(PIN_TFT_CS, HIGH);
                        uploadFile.close();
                        xSemaphoreGive(mutex);
                    }
                } else {
                    uploadFile.close();
                }
            }
            Serial.printf("[Upload] Wi-Fi file upload complete: %s (%u bytes)\n", filename.c_str(), index + len);
            char toastMsg[32];
            snprintf(toastMsg, sizeof(toastMsg), "Saved %s", filename.c_str());
            RenderEngine::getInstance().showToast("FILE UPLOAD", toastMsg, COLOR_NEON_MINT, 2500);
        }
    });

    // 13. OTA Status API (GET /api/ota/status)
    _server.on("/api/ota/status", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["updating"] = OtaHandler::getInstance().isUpdating();
        doc["progress"] = OtaHandler::getInstance().getProgressPercent();
        doc["status"] = OtaHandler::getInstance().getStatusText();
        String res;
        serializeJson(doc, res);
        request->send(200, "application/json", res);
    });

    // 14. Show AI Answer on Device Screen (POST /api/ai/answer)
    _server.on("/api/ai/answer", HTTP_POST, [](AsyncWebServerRequest* request) {
        String question = request->hasParam("question", true) ? request->getParam("question", true)->value() : "AI Question";
        String answer = request->hasParam("answer", true) ? request->getParam("answer", true)->value() : "";
        if (answer.isEmpty() && request->hasParam("answer")) {
            answer = request->getParam("answer")->value();
        }
        if (answer.isEmpty()) {
            request->send(400, "application/json", "{\"error\":\"Missing answer parameter\"}");
            return;
        }
        RenderEngine::getInstance().showAiAnswer(question.c_str(), answer.c_str());
        request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"AI answer displayed on device\"}");
    });

    // 15. Get / Clear AI Answer (GET /api/ai/answer, POST /api/ai/clear)
    _server.on("/api/ai/answer", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["active"] = RenderEngine::getInstance().isAiAnswerActive();
        doc["question"] = RenderEngine::getInstance().getAiQuestion();
        doc["answer"] = RenderEngine::getInstance().getAiAnswer();
        String res;
        serializeJson(doc, res);
        request->send(200, "application/json", res);
    });

    _server.on("/api/ai/clear", HTTP_POST, [](AsyncWebServerRequest* request) {
        RenderEngine::getInstance().hideAiAnswer();
        request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"AI answer dismissed\"}");
    });
}
