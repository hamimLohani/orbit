#include "src/sensors/dht11_sensor.h"

void Dht11Sensor::init() {
    pinMode(PIN_DHT11, INPUT_PULLUP);
    _lastReadTime = millis() - SENSOR_POLL_DHT_MS;
}

bool Dht11Sensor::readRaw(uint8_t data[5]) {
    memset(data, 0, 5);

    // 1. Send start signal: pull low for 20ms
    pinMode(PIN_DHT11, OUTPUT_OPEN_DRAIN);
    digitalWrite(PIN_DHT11, LOW);
    delay(20);

    // Release line & switch to input pullup
    digitalWrite(PIN_DHT11, HIGH);
    delayMicroseconds(30);
    pinMode(PIN_DHT11, INPUT_PULLUP);

    // Microsecond timeout helper — never loops infinitely, never crashes FreeRTOS WDT
    auto waitWhile = [](uint8_t level, uint32_t maxUs) -> int32_t {
        uint32_t start = micros();
        while (digitalRead(PIN_DHT11) == level) {
            if ((micros() - start) > maxUs) return -1;
        }
        return (int32_t)(micros() - start);
    };

    // 2. Sensor response: 80us low, then 80us high
    if (waitWhile(HIGH, 150) < 0) return false;
    if (waitWhile(LOW, 150) < 0) return false;

    // 3. Read 40 data bits
    for (int i = 0; i < 40; ++i) {
        if (waitWhile(HIGH, 100) < 0) return false;
        int32_t highDuration = waitWhile(LOW, 120);
        if (highDuration < 0) return false;

        data[i / 8] <<= 1;
        if (highDuration > 45) {
            data[i / 8] |= 1;
        }
    }

    // Verify 8-bit checksum
    return (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF));
}

bool Dht11Sensor::update() {
    uint32_t now = millis();
    if (now - _lastReadTime < SENSOR_POLL_DHT_MS) {
        return false;
    }
    _lastReadTime = now;

    uint8_t data[5];
    if (!readRaw(data)) {
        _valid = false;
        return false;
    }

    float h = 0.0f;
    float t = 0.0f;

#if (DHT_TYPE == 22 || DHT_TYPE == 21)
    h = ((data[0] << 8) | data[1]) * 0.1f;
    t = (((data[2] & 0x7F) << 8) | data[3]) * 0.1f;
    if (data[2] & 0x80) t = -t;
#else
    h = (float)data[0] + ((float)data[1] * 0.1f);
    t = (float)data[2] + ((float)data[3] * 0.1f);
#endif

    // Filter reasonable terrestrial climate ranges
    if (t >= -10.0f && t <= 60.0f && h >= 5.0f && h <= 100.0f) {
        _lastTemp = (_lastTemp * 0.3f) + (t * 0.7f);
        _lastHum = (_lastHum * 0.3f) + (h * 0.7f);
        _valid = true;
        return true;
    }

    return false;
}
