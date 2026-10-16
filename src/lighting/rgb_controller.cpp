#include "src/lighting/rgb_controller.h"
#include "src/core/shared_data.h"
#include <cmath>

void RgbController::init() {
    pinMode(PIN_RGB_LED, OUTPUT);
    writePixel(0, 0, 0);
}

void RgbController::writePixel(uint8_t r, uint8_t g, uint8_t b) {
    // Apply global brightness scaling
    r = (r * _brightness) / 255;
    g = (g * _brightness) / 255;
    b = (b * _brightness) / 255;
    neopixelWrite(PIN_RGB_LED, r, g, b);
}

void RgbController::setMode(RgbLightingMode mode) {
    _mode = mode;
    SharedData::getInstance().setLighting(_mode, _customColor, _brightness);
}

void RgbController::setColor(uint32_t rgb) {
    _customColor = rgb;
    SharedData::getInstance().setLighting(_mode, _customColor, _brightness);
}

void RgbController::setBrightness(uint8_t brightness) {
    _brightness = brightness;
    SharedData::getInstance().setLighting(_mode, _customColor, _brightness);
}

void RgbController::triggerFlash(uint32_t color, uint16_t durationMs) {
    _isFlashing = true;
    _flashColor = color;
    _flashExpiry = millis() + durationMs;
}

void RgbController::update() {
    uint32_t now = millis();
    if (now - _lastUpdate < 25) return; // 40Hz lighting refresh
    _lastUpdate = now;

    // Handle momentary flash override (e.g. Find My Device, Notification, Beat)
    if (_isFlashing) {
        if (now < _flashExpiry) {
            uint8_t r = (_flashColor >> 16) & 0xFF;
            uint8_t g = (_flashColor >> 8) & 0xFF;
            uint8_t b = _flashColor & 0xFF;
            writePixel(r, g, b);
            return;
        } else {
            _isFlashing = false;
        }
    }

    TelemetryData telem = SharedData::getInstance().getData();

    switch (_mode) {
        case RGB_OFF:
            writePixel(0, 0, 0);
            break;

        case RGB_AMBIENT: {
            // Color smoothly maps to temperature
            float temp = telem.temperature;
            uint8_t r = 0, g = 0, b = 0;
            if (temp < 18.0f) {
                // Cool Cyan-Blue
                r = 0; g = 180; b = 255;
            } else if (temp < 25.0f) {
                // Cozy Emerald Green
                r = 0; g = 255; b = 150;
            } else if (temp < 28.0f) {
                // Warm Amber Gold
                r = 255; g = 190; b = 30;
            } else {
                // Hot Coral / Crimson
                r = 255; g = 60; b = 30;
            }

            // Subtle breathing pulse
            _breathePhase += 0.05f;
            if (_breathePhase > 6.283f) _breathePhase -= 6.283f;
            float factor = 0.5f + 0.5f * sin(_breathePhase);
            writePixel((uint8_t)(r * factor), (uint8_t)(g * factor), (uint8_t)(b * factor));
            break;
        }

        case RGB_DISCO: {
            // Sound-reactive: hue cycles, brightness pulses with audio decibels / bass
            float dbNorm = (telem.micDb - 35.0f) / 55.0f;
            if (dbNorm < 0.05f) dbNorm = 0.05f;
            if (dbNorm > 1.0f) dbNorm = 1.0f;

            _hue = (_hue + 3) % 360;
            uint8_t r, g, b;
            hsvToRgb(_hue, 255, (uint8_t)(dbNorm * 255.0f), r, g, b);
            writePixel(r, g, b);
            break;
        }

        case RGB_BREATHE: {
            _breathePhase += 0.06f;
            if (_breathePhase > 6.283f) _breathePhase -= 6.283f;
            float factor = 0.2f + 0.8f * ((sin(_breathePhase) + 1.0f) / 2.0f);

            uint8_t r = ((_customColor >> 16) & 0xFF) * factor;
            uint8_t g = ((_customColor >> 8) & 0xFF) * factor;
            uint8_t b = (_customColor & 0xFF) * factor;
            writePixel(r, g, b);
            break;
        }

        case RGB_RAINBOW: {
            _hue = (_hue + 2) % 360;
            uint8_t r, g, b;
            hsvToRgb(_hue, 255, 255, r, g, b);
            writePixel(r, g, b);
            break;
        }

        case RGB_SOLID: {
            uint8_t r = (_customColor >> 16) & 0xFF;
            uint8_t g = (_customColor >> 8) & 0xFF;
            uint8_t b = _customColor & 0xFF;
            writePixel(r, g, b);
            break;
        }

        case RGB_ALERT: {
            bool on = (now / 200) % 2;
            writePixel(on ? 255 : 0, 0, 0);
            break;
        }

        default:
            writePixel(0, 0, 0);
            break;
    }
}

void RgbController::hsvToRgb(uint16_t h, uint8_t s, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b) {
    if (s == 0) {
        r = g = b = v;
        return;
    }
    uint8_t region = h / 60;
    uint8_t remainder = (h - (region * 60)) * 6;
    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:  r = v; g = t; b = p; break;
        case 1:  r = q; g = v; b = p; break;
        case 2:  r = p; g = v; b = t; break;
        case 3:  r = p; g = q; b = v; break;
        case 4:  r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
}
