#pragma once
#include <Arduino.h>

// ==============================================================================
// ORBIT v2 — Sensor Subsystem Manager
// ==============================================================================

class SensorManager {
public:
    static SensorManager& getInstance() {
        static SensorManager instance;
        return instance;
    }

    void init();
    void updateSensors(); // Called regularly from OS loop

private:
    SensorManager() = default;
    ~SensorManager() = default;

    uint32_t _lastDhtUpdate = 0;
    uint32_t _lastMicUpdate = 0;
};
