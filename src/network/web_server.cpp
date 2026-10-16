#include "src/network/web_server.h"
#include "src/network/websocket_server.h"
#include "src/core/shared_data.h"
#include "src/core/app_registry.h"
#include "src/core/settings_manager.h"
#include "src/display/bitmap_loader.h"
#include "src/display/render_engine.h"
#include <ArduinoJson.h>

WebServer::WebServer() : _server(HTTP_PORT) {}

void WebServer::init() {
    _fsMounted = LittleFS.begin(true); // Format if mount fails

    // Attach WebSocket
    WebSocketServer::getInstance().init(_server);

    setupRoutes();
    setupRestApi();

    _server.begin();
}

void WebServer::setupRoutes() {
    if (_fsMounted) {
        // Serve LittleFS web directory
        _server.serveStatic("/", LittleFS, "/web/").setDefaultFile("index.html");
    }

    // Fallback landing page if LittleFS is empty
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        String html = "<!DOCTYPE html><html><head><title>ORBIT v2</title>"
                      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                      "<style>body{background:#0a0e17;color:#00ffff;font-family:sans-serif;text-align:center;padding:50px;}</style></head>"
                      "<body><h1>ORBIT v2 System Active</h1><p>WebSocket server listening on /ws</p>"
                      "<p><a style='color:#b81fdf' href='/api/telemetry'>View Telemetry JSON</a></p></body></html>";
        request->send(200, "text/html", html);
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
}
