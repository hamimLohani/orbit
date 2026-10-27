#include "src/lighting/rgb_controller.h"
#include "src/core/shared_data.h"
#include "src/core/settings_manager.h"
#include <cmath>

void RgbController::init() {
    pinMode(PIN_RGB_LED, OUTPUT);
    _mode = static_cast<RgbLightingMode>(SettingsManager::getInstance().getRgbMode());
    _brightness = SettingsManager::getInstance().getAppInt("rgb", "bright", 180);
    _customColor = SettingsManager::getInstance().getRgbColor();
    writePixel(0, 0, 0);
}

void RgbController::writePixel(uint8_t r, uint8_t g, uint8_t b) {
    if (_mode == RGB_OFF && !_isFlashing) {
        neopixelWrite(PIN_RGB_LED, 0, 0, 0);
        return;
    }
    // Apply global brightness scaling
    r = (r * _brightness) / 255;
    g = (g * _brightness) / 255;
    b = (b * _brightness) / 255;
    neopixelWrite(PIN_RGB_LED, r, g, b);
}

void RgbController::setMode(RgbLightingMode mode) {
    _mode = mode;
    if (_mode == RGB_OFF) {
        _isFlashing = false;
        neopixelWrite(PIN_RGB_LED, 0, 0, 0);
    }
    SettingsManager::getInstance().setRgbMode(mode);
    SharedData::getInstance().setLighting(_mode, _customColor, _brightness);
}

void RgbController::setColor(uint32_t rgb) {
    _customColor = rgb;
    SettingsManager::getInstance().setRgbColor(rgb);
    SharedData::getInstance().setLighting(_mode, _customColor, _brightness);
}

void RgbController::setBrightness(uint8_t brightness) {
    _brightness = brightness;
    SettingsManager::getInstance().setAppInt("rgb", "bright", brightness);
    SharedData::getInstance().setLighting(_mode, _customColor, _brightness);
}

void RgbController::triggerFlash(uint32_t color, uint16_t durationMs) {
    // Only flash if the light has been turned on in the Lights section
    if (_mode == RGB_OFF) return;
    _isFlashing = true;
    _flashColor = color;
    _flashExpiry = millis() + durationMs;
}

void RgbController::update() {
    uint32_t now = millis();
    if (now - _lastUpdate < 25) return; // 40Hz lighting refresh
    _lastUpdate = now;

    if (_mode == RGB_OFF) {
        _isFlashing = false;
        neopixelWrite(PIN_RGB_LED, 0, 0, 0);
        return;
    }

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

        case RGB_DISCO_ALERT: {
            // Sound-reactive beat strobe & party alert
            // Reacts dynamically to bass energy (FFT bands 0-1) and sound pressure (micDb)
            bool isBassBeat = (telem.fftBands[0] > 120 || telem.fftBands[1] > 120);
            bool isLoudAlert = (telem.micDb > 62.0f || telem.noiseDistraction);

            if (isBassBeat || isLoudAlert) {
                // High-energy chromatic strobe: fast bursts through intense neon hues
                uint8_t strobeStep = (now / 40) % 4;
                switch (strobeStep) {
                    case 0: writePixel(255, 0, 90); break;    // Neon Hot Pink / Red
                    case 1: writePixel(0, 240, 255); break;   // Electric Cyan
                    case 2: writePixel(255, 230, 20); break;  // Strobe Gold
                    case 3: writePixel(255, 255, 255); break; // Blinding White Flash
                }
            } else {
                // Dynamic audio-sensitive groove: hue cycles faster with volume
                float dbNorm = (telem.micDb - 35.0f) / 45.0f;
                if (dbNorm < 0.05f) dbNorm = 0.05f;
                if (dbNorm > 1.0f) dbNorm = 1.0f;

                uint8_t hueStep = 2 + (uint8_t)(dbNorm * 6.0f);
                _hue = (_hue + hueStep) % 360;
                uint8_t val = 45 + (uint8_t)(dbNorm * 210.0f);
                uint8_t r, g, b;
                hsvToRgb(_hue, 255, val, r, g, b);
                writePixel(r, g, b);
            }
            break;
        }

        case RGB_CANDLE: {
            // Organic warm flickering flame / campfire
            static uint32_t lastFlickerTime = 0;
            static uint8_t flickerJitter = 180;
            if (now - lastFlickerTime > 40) {
                lastFlickerTime = now;
                flickerJitter = 135 + (rand() % 120); // 135 to 254
            }
            _breathePhase += 0.035f;
            if (_breathePhase > 6.283f) _breathePhase -= 6.283f;
            float slowSway = 0.82f + 0.18f * sin(_breathePhase);

            uint8_t v = (uint8_t)(flickerJitter * slowSway);
            // Flame color palette: Red=max, Green=110..130, Blue=10..20
            uint8_t r = v;
            uint8_t g = (v * 125) / 255;
            uint8_t b = (v * 15) / 255;
            writePixel(r, g, b);
            break;
        }

        case RGB_AURORA: {
            // Ethereal northern lights: slow drifting wave between mint green, cyan, and violet
            _breathePhase += 0.015f;
            if (_breathePhase > 6.283f) _breathePhase -= 6.283f;

            // Oscillate hue between 135 (mint green) and 280 (violet)
            float wave = 0.5f + 0.5f * sin(_breathePhase);
            uint16_t auroraHue = 135 + (uint16_t)(wave * 145.0f);

            float brightnessWave = 0.7f + 0.3f * sin(_breathePhase * 2.1f);
            uint8_t v = (uint8_t)(255 * brightnessWave);

            uint8_t r, g, b;
            hsvToRgb(auroraHue, 240, v, r, g, b);
            writePixel(r, g, b);
            break;
        }

        case RGB_POLICE: {
            // High-visibility dual-burst emergency strobe (Red / Blue)
            uint16_t cycle = now % 700;
            if ((cycle >= 0 && cycle < 80) || (cycle >= 140 && cycle < 220)) {
                writePixel(255, 0, 0); // Burst Red
            } else if ((cycle >= 350 && cycle < 430) || (cycle >= 490 && cycle < 570)) {
                writePixel(0, 50, 255); // Burst Blue
            } else {
                writePixel(0, 0, 0); // Strobe gap
            }
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
