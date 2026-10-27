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

    bool isUpdating() const { return _isUpdating; }
    uint8_t getProgressPercent() const { return _progressPercent; }
    const char* getStatusText() const { return _statusText; }
    void setUpdating(bool updating, uint8_t percent = 0, const char* status = "");

private:
    OtaHandler() = default;
    ~OtaHandler() = default;

    bool _initialized = false;
    bool _isUpdating = false;
    uint8_t _progressPercent = 0;
    char _statusText[32] = "Ready";
};
