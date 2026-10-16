#pragma once
#include <Arduino.h>
#include <ArduinoOTA.h>

// ==============================================================================
// ORBIT v2 — Over-The-Air (OTA) Firmware Update Handler
// ==============================================================================

class OtaHandler {
public:
    static OtaHandler& getInstance() {
        static OtaHandler instance;
        return instance;
    }

    void init();
    void update(); // Called frequently from network task

private:
    OtaHandler() = default;
    ~OtaHandler() = default;

    bool _initialized = false;
};
