#include "src/network/web_server.h"
#include "src/network/websocket_server.h"
#include "src/network/wifi_manager.h"
#include "src/network/wifi_setup_page.h"
#include "src/core/shared_data.h"
#include "src/core/app_registry.h"
#include "src/core/settings_manager.h"
#include "src/display/bitmap_loader.h"
#include "src/display/render_engine.h"
#include <ArduinoJson.h>
#include <esp_system.h>
#include <esp_idf_version.h>

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
        request->send_P(200, "text/html", WIFI_SETUP_HTML);
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

    // 3. Serve LittleFS Web Companion SPA if mounted
    if (_fsMounted) {
        _server.serveStatic("/", LittleFS, "/web/").setDefaultFile("index.html");
    }

    // 4. Root / Landing Page Handler
    _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        // If in SoftAP mode and not connected to a local router, serve the Wi-Fi setup page
        if (WifiManager::getInstance().isApActive() && !WifiManager::getInstance().isConnected()) {
            request->send_P(200, "text/html", WIFI_SETUP_HTML);
            return;
        }

        // If LittleFS has index.html, serve the full companion dashboard
        if (_fsMounted && LittleFS.exists("/web/index.html")) {
            request->send(LittleFS, "/web/index.html", "text/html");
            return;
        }

        // Fallback: serve the self-contained setup page
        request->send_P(200, "text/html", WIFI_SETUP_HTML);
    });
}

void WebServer::setupRestApi() {
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

    // 5. Wi-Fi Scan Results API (GET /api/wifi/scan)
    _server.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!WifiManager::getInstance().isScanning() && WifiManager::getInstance().getScannedNetworks().empty()) {
            WifiManager::getInstance().startScan();
        }
        String res = WifiManager::getInstance().getScannedNetworksJson();
        request->send(200, "application/json", res);
    });

    // 6. Trigger New Wi-Fi Scan (POST /api/wifi/scan)
    _server.on("/api/wifi/scan", HTTP_POST, [](AsyncWebServerRequest* request) {
        WifiManager::getInstance().startScan();
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
}
