#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "include/config.h"
#include "src/display/theme.h"
#include "include/assets/icons.h"

// ==============================================================================
// ORBIT v2 — Graphics & Render Engine (ST7735 128x160)
// ==============================================================================

struct ToastMessage {
    bool active = false;
    char title[24];
    char message[48];
    uint16_t color;
    uint32_t expiryTime;
};

class RenderEngine {
public:
    static RenderEngine& getInstance() {
        static RenderEngine instance;
        return instance;
    }

    void init();
    void setBacklight(uint8_t brightnessPercent); // 0-100%

    // Fast screen clears
    void fillScreen(uint16_t color);

    // Primitives
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
    void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

    // Advanced UI Primitives
    void drawCard(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t bg, uint16_t border);
    void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t percent, uint16_t fg, uint16_t bg = COLOR_SLATE_DARK);
    void drawSparkline(int16_t x, int16_t y, int16_t w, int16_t h, const float* data, size_t count, float minVal, float maxVal, uint16_t color);
    void drawIcon(int16_t x, int16_t y, uint8_t iconIndex, uint16_t color, uint16_t bg = COLOR_VOID_DARK);
    void drawBitmap16x16(int16_t x, int16_t y, const uint8_t* bitmap, uint16_t color, uint16_t bg);

    // Typography
    void drawText(int16_t x, int16_t y, const char* text, uint16_t color = COLOR_WHITE, uint8_t size = 1);
    void drawTextCentered(int16_t xCenter, int16_t y, const char* text, uint16_t color = COLOR_WHITE, uint8_t size = 1);
    void drawTextRight(int16_t xRight, int16_t y, const char* text, uint16_t color = COLOR_WHITE, uint8_t size = 1);

    // System Components
    void drawStatusBar(const char* timeStr, bool wifiConnected, int8_t rssi, uint8_t activeIcon);
    void drawHeader(const char* title, uint8_t iconIndex = 255);

    // Toast Notifications
    void showToast(const char* title, const char* msg, uint16_t color = COLOR_NEON_CYAN, uint32_t durationMs = 3000);
    void renderToastOverlay();

    // Flush the double-buffered frame to the physical display in one SPI transaction
    void flush();

    // Hardware SPI Mutex Access
    static SemaphoreHandle_t getSpiMutex();

    Adafruit_ST7735& getTft() { return _tft; }
    GFXcanvas16& getCanvas() { return _canvas; }

private:
    RenderEngine();
    ~RenderEngine() = default;

    Adafruit_ST7735 _tft;
    GFXcanvas16 _canvas;
    ToastMessage _toast;
    static SemaphoreHandle_t _spiMutex;
};
