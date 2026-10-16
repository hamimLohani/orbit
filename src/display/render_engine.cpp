#include "src/display/render_engine.h"
#include <cstring>

SemaphoreHandle_t RenderEngine::_spiMutex = nullptr;

RenderEngine::RenderEngine() 
    : _tft(&SPI, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST),
      _canvas(SCREEN_WIDTH, SCREEN_HEIGHT) {
    if (!_spiMutex) {
        _spiMutex = xSemaphoreCreateMutex();
    }
}

SemaphoreHandle_t RenderEngine::getSpiMutex() {
    if (!_spiMutex) {
        _spiMutex = xSemaphoreCreateMutex();
    }
    return _spiMutex;
}

void RenderEngine::init() {
    // Backlight PWM pin - start solid ON
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);

    // Initialize SPI bus with custom pins
    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);

    if (xSemaphoreTake(getSpiMutex(), portMAX_DELAY) == pdTRUE) {
        // Deselect SD card during TFT initialization
        pinMode(PIN_SD_CS, OUTPUT);
        digitalWrite(PIN_SD_CS, HIGH);

        _tft.initR(INITR_BLACKTAB); // Standard ST7735 1.8" 128x160
        _tft.setRotation(0);        // 128x160 Portrait
        _tft.fillScreen(COLOR_VOID_DARK);
        xSemaphoreGive(getSpiMutex());
    }

    _canvas.fillScreen(COLOR_VOID_DARK);
    setBacklight(100);
}

void RenderEngine::setBacklight(uint8_t brightnessPercent) {
    brightnessPercent = constrain(brightnessPercent, 0, 100);
    if (brightnessPercent >= 99) {
        digitalWrite(PIN_TFT_BL, HIGH);
    } else if (brightnessPercent == 0) {
        digitalWrite(PIN_TFT_BL, LOW);
    } else {
        uint8_t duty = map(brightnessPercent, 0, 100, 0, 255);
        analogWrite(PIN_TFT_BL, duty);
    }
}

void RenderEngine::flush() {
    if (xSemaphoreTake(getSpiMutex(), pdMS_TO_TICKS(50)) == pdTRUE) {
        digitalWrite(PIN_SD_CS, HIGH); // Ensure SD is deselected
        _tft.drawRGBBitmap(0, 0, _canvas.getBuffer(), SCREEN_WIDTH, SCREEN_HEIGHT);
        xSemaphoreGive(getSpiMutex());
    }
}

// In-Memory Double-Buffered Primitives (Fast & Flicker-Free)
void RenderEngine::fillScreen(uint16_t color) {
    _canvas.fillScreen(color);
}

void RenderEngine::drawPixel(int16_t x, int16_t y, uint16_t color) {
    _canvas.drawPixel(x, y, color);
}

void RenderEngine::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    _canvas.drawLine(x0, y0, x1, y1, color);
}

void RenderEngine::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    _canvas.drawFastHLine(x, y, w, color);
}

void RenderEngine::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    _canvas.drawFastVLine(x, y, h, color);
}

void RenderEngine::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    _canvas.drawRect(x, y, w, h, color);
}

void RenderEngine::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    _canvas.fillRect(x, y, w, h, color);
}

void RenderEngine::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    _canvas.drawRoundRect(x, y, w, h, r, color);
}

void RenderEngine::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    _canvas.fillRoundRect(x, y, w, h, r, color);
}

void RenderEngine::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    _canvas.drawCircle(x0, y0, r, color);
}

void RenderEngine::fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    _canvas.fillCircle(x0, y0, r, color);
}

void RenderEngine::drawCard(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t bg, uint16_t border) {
    fillRoundRect(x, y, w, h, 3, bg);
    drawRoundRect(x, y, w, h, 3, border);
}

void RenderEngine::drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t percent, uint16_t fg, uint16_t bg) {
    percent = constrain(percent, 0, 100);
    fillRoundRect(x, y, w, h, 2, bg);
    int16_t fillW = (w * percent) / 100;
    if (fillW > 0) {
        fillRoundRect(x, y, fillW, h, 2, fg);
    }
}

void RenderEngine::drawSparkline(int16_t x, int16_t y, int16_t w, int16_t h, const float* data, size_t count, float minVal, float maxVal, uint16_t color) {
    if (!data || count < 2) return;
    float range = maxVal - minVal;
    if (range <= 0.001f) range = 1.0f;

    float stepX = (float)w / (float)(count - 1);
    for (size_t i = 0; i < count - 1; ++i) {
        int16_t x1 = x + (int16_t)(i * stepX);
        int16_t y1 = y + h - (int16_t)(((data[i] - minVal) / range) * h);
        int16_t x2 = x + (int16_t)((i + 1) * stepX);
        int16_t y2 = y + h - (int16_t)(((data[i + 1] - minVal) / range) * h);
        drawLine(x1, y1, x2, y2, color);
    }
}

void RenderEngine::drawBitmap16x16(int16_t x, int16_t y, const uint8_t* bitmap, uint16_t color, uint16_t bg) {
    if (!bitmap) return;
    _canvas.drawBitmap(x, y, bitmap, 16, 16, color, bg);
}

void RenderEngine::drawIcon(int16_t x, int16_t y, uint8_t iconIndex, uint16_t color, uint16_t bg) {
    if (iconIndex < 12) {
        const uint8_t* iconData = (const uint8_t*)pgm_read_ptr(&(SYSTEM_ICONS[iconIndex]));
        drawBitmap16x16(x, y, iconData, color, bg);
    }
}

void RenderEngine::drawText(int16_t x, int16_t y, const char* text, uint16_t color, uint8_t size) {
    if (!text) return;
    _canvas.setTextColor(color);
    _canvas.setTextSize(size);
    _canvas.setCursor(x, y);
    _canvas.print(text);
}

void RenderEngine::drawTextCentered(int16_t xCenter, int16_t y, const char* text, uint16_t color, uint8_t size) {
    if (!text) return;
    int16_t charWidth = 6 * size;
    int16_t totalWidth = strlen(text) * charWidth;
    int16_t x = xCenter - (totalWidth / 2);
    drawText(x, y, text, color, size);
}

void RenderEngine::drawTextRight(int16_t xRight, int16_t y, const char* text, uint16_t color, uint8_t size) {
    if (!text) return;
    int16_t charWidth = 6 * size;
    int16_t totalWidth = strlen(text) * charWidth;
    int16_t x = xRight - totalWidth;
    drawText(x, y, text, color, size);
}

void RenderEngine::drawStatusBar(const char* timeStr, bool wifiConnected, int8_t rssi, uint8_t activeIcon) {
    fillRect(0, 0, SCREEN_WIDTH, STATUS_BAR_HEIGHT, COLOR_VOID_DARK);
    drawFastHLine(0, STATUS_BAR_HEIGHT - 1, SCREEN_WIDTH, COLOR_SLATE_BORDER);

    fillCircle(4, 6, 2, COLOR_NEON_CYAN);

    if (timeStr && strlen(timeStr) >= 5) {
        char shortTime[6];
        strncpy(shortTime, timeStr, 5);
        shortTime[5] = '\0';
        drawTextCentered(64, 3, shortTime, COLOR_TEXT_LIGHT, 1);
    }

    int16_t wx = SCREEN_WIDTH - 16;
    int16_t wy = 9;
    if (wifiConnected) {
        uint16_t wColor = COLOR_NEON_MINT;
        drawFastVLine(wx, wy - 2, 2, wColor);
        drawFastVLine(wx + 3, wy - 4, 4, (rssi > -80) ? wColor : COLOR_TEXT_DARK);
        drawFastVLine(wx + 6, wy - 6, 6, (rssi > -65) ? wColor : COLOR_TEXT_DARK);
    } else {
        drawText(wx, 3, "x", COLOR_NEON_RED, 1);
    }
}

void RenderEngine::drawHeader(const char* title, uint8_t iconIndex) {
    if (iconIndex != 255) {
        drawIcon(4, STATUS_BAR_HEIGHT + 3, iconIndex, COLOR_NEON_CYAN, COLOR_VOID_DARK);
        drawText(24, STATUS_BAR_HEIGHT + 7, title, COLOR_TEXT_LIGHT, 1);
    } else {
        drawTextCentered(64, STATUS_BAR_HEIGHT + 6, title, COLOR_TEXT_LIGHT, 1);
    }
    drawFastHLine(4, STATUS_BAR_HEIGHT + 23, SCREEN_WIDTH - 8, COLOR_SLATE_BORDER);
}

void RenderEngine::showToast(const char* title, const char* msg, uint16_t color, uint32_t durationMs) {
    _toast.active = true;
    strncpy(_toast.title, title, sizeof(_toast.title) - 1);
    strncpy(_toast.message, msg, sizeof(_toast.message) - 1);
    _toast.color = color;
    _toast.expiryTime = millis() + durationMs;
}

void RenderEngine::renderToastOverlay() {
    if (!_toast.active) return;

    if (millis() > _toast.expiryTime) {
        _toast.active = false;
        return;
    }

    fillRect(4, 2, SCREEN_WIDTH - 8, 28, COLOR_SLATE_MID);
    drawRect(4, 2, SCREEN_WIDTH - 8, 28, _toast.color);
    drawFastHLine(6, 14, SCREEN_WIDTH - 12, COLOR_SLATE_BORDER);

    drawText(8, 5, _toast.title, _toast.color, 1);
    drawText(8, 17, _toast.message, COLOR_TEXT_LIGHT, 1);
}
