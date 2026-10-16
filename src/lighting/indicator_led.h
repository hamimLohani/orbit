#pragma once
#include <Arduino.h>
#include "include/config.h"

// ==============================================================================
// ORBIT v2 — Dedicated Indicator & Notification LED
// ==============================================================================

enum IndicatorPattern {
    LED_OFF = 0,
    LED_SOLID,
    LED_HEARTBEAT,      // Double pulse every 1.5s
    LED_BLINK_SLOW,     // 1Hz blink
    LED_BLINK_FAST,     // 5Hz alert strobe
    LED_SINGLE_FLASH    // Momentary notification chirp
};

class IndicatorLed {
public:
    static IndicatorLed& getInstance() {
        static IndicatorLed instance;
        return instance;
    }

    void init();
    void update(); // Called frequently from OS loop

    void setPattern(IndicatorPattern pattern);
    void flashOnce(uint16_t durationMs = 150);

private:
    IndicatorLed() = default;
    ~IndicatorLed() = default;

    IndicatorPattern _pattern = LED_OFF;
    bool _isFlashing = false;
    uint32_t _flashExpiry = 0;
};
