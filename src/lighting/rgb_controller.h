#pragma once
#include <Arduino.h>
#include "include/config.h"

// ==============================================================================
// ORBIT v2 — WS2812 Addressable RGB Controller
// ==============================================================================

enum RgbLightingMode {
    RGB_OFF = 0,
    RGB_AMBIENT,       // Temperature/climate reactive
    RGB_DISCO,         // Sound & FFT reactive
    RGB_BREATHE,       // Sinusoidal breathing pulse
    RGB_RAINBOW,       // Smooth HSV hue rotation
    RGB_SOLID,         // Static custom color
    RGB_ALERT,         // Flashing red alert
    RGB_MODE_MAX
};

class RgbController {
public:
    static RgbController& getInstance() {
        static RgbController instance;
        return instance;
    }

    void init();
    void update(); // Called frequently from OS loop

    void setMode(RgbLightingMode mode);
    RgbLightingMode getMode() const { return _mode; }

    void setColor(uint32_t rgb);
    uint32_t getColor() const { return _customColor; }

    void setBrightness(uint8_t brightness); // 0-255
    uint8_t getBrightness() const { return _brightness; }

    void triggerFlash(uint32_t color, uint16_t durationMs = 250);

private:
    RgbController() = default;
    ~RgbController() = default;

    void writePixel(uint8_t r, uint8_t g, uint8_t b);
    void hsvToRgb(uint16_t h, uint8_t s, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b);

    RgbLightingMode _mode = RGB_OFF;
    uint32_t _customColor = 0x00FFFF;
    uint8_t _brightness = 180;
    uint32_t _lastUpdate = 0;
    uint16_t _hue = 0;
    float _breathePhase = 0.0f;

    // Flash override
    bool _isFlashing = false;
    uint32_t _flashColor = 0xFFFFFF;
    uint32_t _flashExpiry = 0;
};
