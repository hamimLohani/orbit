#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "include/config.h"

// ==============================================================================
// ORBIT v2 — Wi-Fi & mDNS Manager
// ==============================================================================

class WifiManager {
public:
    static WifiManager& getInstance() {
        static WifiManager instance;
        return instance;
    }

    void init();
    void update(); // Called from Core 0 network task

    bool isConnected() const;
    int8_t getRssi() const;
    String getIpAddress() const;
    void connect(const String& ssid, const String& pass);

private:
    WifiManager() = default;
    ~WifiManager() = default;

    void startStation();
    void startApFallback();

    bool _apActive = false;
    uint32_t _lastCheckTime = 0;
    uint32_t _connectStartTime = 0;
    bool _connecting = false;
};
