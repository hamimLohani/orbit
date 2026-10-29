#pragma once
#include <Arduino.h>
#include "include/config.h"

// ==============================================================================
// ORBIT v2 — Non-Blocking Native DHT Climate Sensor (Watchdog-Safe)
// ==============================================================================

class Dht11Sensor {
public:
    static Dht11Sensor& getInstance() {
        static Dht11Sensor instance;
        return instance;
    }

    void init();
    bool update(); // Non-blocking check, returns true if new reading acquired

    float getTemperature() const { return _lastTemp; }
    float getHumidity() const { return _lastHum; }
    bool isValid() const { return _valid; }

private:
    Dht11Sensor() = default;
    ~Dht11Sensor() = default;

    bool readRaw(uint8_t data[5]);

    uint32_t _lastReadTime = 0;
    float _lastTemp = 24.0f;
    float _lastHum = 50.0f;
    bool _valid = false;
};
