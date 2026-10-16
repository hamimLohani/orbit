#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// ==============================================================================
// ORBIT v2 — Async Web Server & REST API Endpoints
// ==============================================================================

class WebServer {
public:
    static WebServer& getInstance() {
        static WebServer instance;
        return instance;
    }

    void init();
    AsyncWebServer& getServer() { return _server; }

private:
    WebServer();
    ~WebServer() = default;

    void setupRoutes();
    void setupRestApi();

    AsyncWebServer _server;
    bool _fsMounted = false;
};
