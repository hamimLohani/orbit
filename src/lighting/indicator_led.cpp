#include "src/lighting/indicator_led.h"

void IndicatorLed::init() {
    pinMode(PIN_INDICATOR_LED, OUTPUT);
    digitalWrite(PIN_INDICATOR_LED, LOW);
}

void IndicatorLed::setPattern(IndicatorPattern pattern) {
    _pattern = pattern;
}

void IndicatorLed::flashOnce(uint16_t durationMs) {
    _isFlashing = true;
    _flashExpiry = millis() + durationMs;
    digitalWrite(PIN_INDICATOR_LED, HIGH);
}

void IndicatorLed::update() {
    uint32_t now = millis();

    if (_isFlashing) {
        if (now >= _flashExpiry) {
            _isFlashing = false;
            digitalWrite(PIN_INDICATOR_LED, LOW);
        }
        return;
    }

    switch (_pattern) {
        case LED_OFF:
            digitalWrite(PIN_INDICATOR_LED, LOW);
            break;

        case LED_SOLID:
            digitalWrite(PIN_INDICATOR_LED, HIGH);
            break;

        case LED_HEARTBEAT: {
            uint32_t phase = now % 1500;
            // Beat 1: 0-100ms, Beat 2: 250-350ms, then off
            bool on = (phase < 100) || (phase >= 250 && phase < 350);
            digitalWrite(PIN_INDICATOR_LED, on ? HIGH : LOW);
            break;
        }

        case LED_BLINK_SLOW: {
            bool on = (now / 500) % 2;
            digitalWrite(PIN_INDICATOR_LED, on ? HIGH : LOW);
            break;
        }

        case LED_BLINK_FAST: {
            bool on = (now / 100) % 2;
            digitalWrite(PIN_INDICATOR_LED, on ? HIGH : LOW);
            break;
        }

        default:
            digitalWrite(PIN_INDICATOR_LED, LOW);
            break;
    }
}
