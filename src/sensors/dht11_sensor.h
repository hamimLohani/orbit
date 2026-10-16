#pragma once
#include <Arduino.h>
#include <DHT.h>
#include "include/config.h"

// ==============================================================================
// ORBIT v2 — Non-Blocking DHT11 Climate Sensor
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
    Dht11Sensor();
    ~Dht11Sensor() = default;

    DHT _dht;
    uint32_t _lastReadTime = 0;
    float _lastTemp = 22.0f;
    float _lastHum = 50.0f;
    bool _valid = false;
};
