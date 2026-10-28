#include "src/display/render_engine.h"
#include "src/core/settings_manager.h"
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
    // Backlight PWM pin
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

    // Remember and restore previous user brightness preference from NVS
    uint8_t savedBrightness = SettingsManager::getInstance().getBrightness();
    setBacklight(savedBrightness);
}

void RenderEngine::setBacklight(uint8_t brightnessPercent) {
    brightnessPercent = constrain(brightnessPercent, 0, 100);
    _currentBrightness = brightnessPercent;
    if (brightnessPercent >= 99) {
        digitalWrite(PIN_TFT_BL, HIGH);
    } else {
        // Map 0-100% to [TFT_BACKLIGHT_MIN_DUTY, 255] so 0% dims to minimum readable level and never turns off the screen
        uint8_t duty = map(brightnessPercent, 0, 100, TFT_BACKLIGHT_MIN_DUTY, 255);
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
    if (iconIndex < (sizeof(SYSTEM_ICONS) / sizeof(SYSTEM_ICONS[0]))) {
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

void RenderEngine::showTextOverlay(const char* title, const char* subtitle, const char* text) {
    _aiAnswer.active = true;
    _aiAnswer.scrollOffset = 0;
    strncpy(_aiAnswer.title, (title && strlen(title) > 0) ? title : "TRANSCRIPT", sizeof(_aiAnswer.title) - 1);
    _aiAnswer.title[sizeof(_aiAnswer.title) - 1] = '\0';
    strncpy(_aiAnswer.question, (subtitle && strlen(subtitle) > 0) ? subtitle : "Voice Note", sizeof(_aiAnswer.question) - 1);
    _aiAnswer.question[sizeof(_aiAnswer.question) - 1] = '\0';
    strncpy(_aiAnswer.answer, (text && strlen(text) > 0) ? text : "No text transcribed.", sizeof(_aiAnswer.answer) - 1);
    _aiAnswer.answer[sizeof(_aiAnswer.answer) - 1] = '\0';
}

void RenderEngine::showAiAnswer(const char* question, const char* answer) {
    showTextOverlay("AI ANSWER", (question && strlen(question) > 0) ? question : "AI Prompt", answer);
}

void RenderEngine::hideAiAnswer() {
    _aiAnswer.active = false;
}

void RenderEngine::scrollAiAnswer(int8_t dir) {
    if (!_aiAnswer.active) return;
    int16_t next = _aiAnswer.scrollOffset + dir;
    if (next < 0) next = 0;
    if (next > 30) next = 30;
    _aiAnswer.scrollOffset = next;
}

void RenderEngine::renderAiAnswerOverlay() {
    if (!_aiAnswer.active) return;

    // Full screen backdrop
    fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_VOID_DARK);

    // Frame border
    drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_NEON_CYAN);

    // Header bar (y=0..18)
    fillRect(1, 1, SCREEN_WIDTH - 2, 17, COLOR_SLATE_MID);
    drawText(6, 5, _aiAnswer.title, COLOR_NEON_CYAN, 1);
    drawTextRight(122, 5, "ORBIT", COLOR_NEON_MINT, 1);
    drawFastHLine(1, 18, SCREEN_WIDTH - 2, COLOR_NEON_CYAN);

    // Subtitle / Question Box (y=21..44, h=23, x=4, w=120)
    drawCard(4, 21, 120, 23, COLOR_SLATE_DARK, COLOR_SLATE_BORDER);
    char qBuf[22];
    if (strcmp(_aiAnswer.title, "AI ANSWER") == 0) {
        snprintf(qBuf, sizeof(qBuf), "Q: %s", _aiAnswer.question);
    } else {
        snprintf(qBuf, sizeof(qBuf), "> %s", _aiAnswer.question);
    }
    drawText(8, 27, qBuf, COLOR_NEON_AMBER, 1);

    // Answer Card (y=47..143, h=96, x=4, w=120)
    drawCard(4, 47, 120, 96, COLOR_SLATE_MID, COLOR_SLATE_BORDER);

    // Word-wrap lines for 18 characters maximum width
    const int maxCharsPerLine = 18;
    const int maxVisibleLines = 8;
    const int lineH = 11;
    const int startY = 52;

    const char* p = _aiAnswer.answer;
    int lineIndex = 0;
    int renderedLines = 0;

    while (*p && renderedLines < maxVisibleLines) {
        char lineBuf[maxCharsPerLine + 1];
        int len = 0;

        // Skip leading spaces or empty newlines
        while (*p == ' ' || *p == '\n') {
            if (*p == '\n') { p++; break; }
            p++;
        }
        if (!*p) break;

        // Read up to maxCharsPerLine, tracking last space
        int lastSpace = -1;
        while (*p && *p != '\n' && len < maxCharsPerLine) {
            lineBuf[len] = *p;
            if (*p == ' ') lastSpace = len;
            len++;
            p++;
        }

        // Backtrack to last space if word exceeds line boundary
        if (len == maxCharsPerLine && *p && *p != ' ' && *p != '\n' && lastSpace > 4) {
            int rewind = len - lastSpace;
            p -= rewind;
            len = lastSpace;
        }
        lineBuf[len] = '\0';

        if (lineIndex >= _aiAnswer.scrollOffset) {
            drawText(8, startY + (renderedLines * lineH), lineBuf, COLOR_TEXT_LIGHT, 1);
            renderedLines++;
        }
        lineIndex++;
    }

    // Scroll indicators if applicable
    if (_aiAnswer.scrollOffset > 0) {
        drawTextRight(120, 50, "^", COLOR_NEON_CYAN, 1);
    }
    if (*p) {
        drawTextRight(120, 133, "v", COLOR_NEON_CYAN, 1);
    }

    // Bottom Navigation Hint (y=147)
    drawTextCentered(64, 147, "UP/DN:Scroll  OK:Close", COLOR_TEXT_MUTED, 1);
}
