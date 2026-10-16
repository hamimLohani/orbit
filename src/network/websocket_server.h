#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// ==============================================================================
// ORBIT v2 — Async WebSocket Server & Telemetry Broadcaster
// ==============================================================================

class WebSocketServer {
public:
    static WebSocketServer& getInstance() {
        static WebSocketServer instance;
        return instance;
    }

    void init(AsyncWebServer& server);
    void update(); // Called frequently from Core 0 network task

    void broadcastTelemetry();
    void sendToAll(const String& message);

    AsyncWebSocket& getWs() { return _ws; }

private:
    WebSocketServer();
    ~WebSocketServer() = default;

    void handleMessage(AsyncWebSocketClient* client, void* arg, uint8_t* data, size_t len);
    void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);

    AsyncWebSocket _ws;
    uint32_t _lastBroadcast = 0;
};
