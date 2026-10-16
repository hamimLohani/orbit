#include "src/sensors/dht11_sensor.h"

Dht11Sensor::Dht11Sensor() : _dht(PIN_DHT11, DHT_TYPE) {}

void Dht11Sensor::init() {
    _dht.begin();
    _lastReadTime = millis() - SENSOR_POLL_DHT_MS; // Ready immediately
}

bool Dht11Sensor::update() {
    uint32_t now = millis();
    if (now - _lastReadTime < SENSOR_POLL_DHT_MS) {
        return false;
    }
    _lastReadTime = now;

    float h = _dht.readHumidity();
    float t = _dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        _valid = false;
        return false;
    }

    // Filter outliers
    if (t >= -10.0f && t <= 60.0f && h >= 5.0f && h <= 100.0f) {
        // Light smoothing: 70% new, 30% old
        _lastTemp = (_lastTemp * 0.3f) + (t * 0.7f);
        _lastHum = (_lastHum * 0.3f) + (h * 0.7f);
        _valid = true;
        return true;
    }

    return false;
}
