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
    void updateSensors(); // Updates both audio and climate (backward compatibility)
    void updateClimate(); // Called from SensorTask (DHT11)
    void updateAudio();   // Called from AudioTask (I2S Mic & FFT)

private:
    SensorManager() = default;
    ~SensorManager() = default;

    uint32_t _lastDhtUpdate = 0;
    uint32_t _lastMicUpdate = 0;
};
