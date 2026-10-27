#include "src/apps/climate_app.h"
#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/display/theme.h"

REGISTER_APP(ClimateApp);

void ClimateApp::onInit() {
    for (int i = 0; i < CLIMATE_HISTORY_POINTS; ++i) {
        _tempHistory[i] = 22.0f;
    }
}

void ClimateApp::onEnter() {}
void ClimateApp::onExit() {}

void ClimateApp::onUpdate(uint32_t deltaMs) {
    uint32_t now = millis();
    if (now - _lastHistorySample > 30000) { // Record every 30s
        _lastHistorySample = now;
        TelemetryData telem = SharedData::getInstance().getData();

        // Shift left
        for (int i = 0; i < CLIMATE_HISTORY_POINTS - 1; ++i) {
            _tempHistory[i] = _tempHistory[i + 1];
        }
        _tempHistory[CLIMATE_HISTORY_POINTS - 1] = telem.temperature;
    }
}

void ClimateApp::onRender(RenderEngine& gfx) {
    TelemetryData telem = SharedData::getInstance().getData();

    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);
    gfx.drawHeader("CLIMATE MONITOR", 2);

    // --------------------------------------------------------------------------
    // 1. Top Row: Twin Symmetrical Metric Cards (y = 40..86, height 46)
    // --------------------------------------------------------------------------
    
    // Left Card: Temperature (x = 6, width = 56)
    gfx.drawCard(6, STATUS_BAR_HEIGHT + 26, 56, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(11, STATUS_BAR_HEIGHT + 31, "TEMP", COLOR_TEXT_MUTED, 1);

    char tBuf[12];
    snprintf(tBuf, sizeof(tBuf), "%.1f", telem.temperature);
    // Draw large numeric value (size 2)
    gfx.drawText(10, STATUS_BAR_HEIGHT + 43, tBuf, COLOR_NEON_CYAN, 2);
    // Unit badge in corner
    gfx.drawText(46, STATUS_BAR_HEIGHT + 31, "C", COLOR_TEXT_LIGHT, 1);
    gfx.drawPixel(42, STATUS_BAR_HEIGHT + 31, COLOR_NEON_CYAN); // Degree symbol dot

    // Right Card: Humidity (x = 66, width = 56)
    gfx.drawCard(66, STATUS_BAR_HEIGHT + 26, 56, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(71, STATUS_BAR_HEIGHT + 31, "HUMID", COLOR_TEXT_MUTED, 1);

    char hBuf[12];
    snprintf(hBuf, sizeof(hBuf), "%.0f%%", telem.humidity);
    gfx.drawText(70, STATUS_BAR_HEIGHT + 43, hBuf, COLOR_NEON_MINT, 2);
    // Live mini humidity level bar
    uint8_t humPct = (uint8_t)constrain((int)telem.humidity, 0, 100);
    gfx.drawProgressBar(71, STATUS_BAR_HEIGHT + 61, 46, 4, humPct, COLOR_NEON_MINT);

    // --------------------------------------------------------------------------
    // 2. Middle Row: Environmental Comfort Badge Pill (y = 90..110, height 20)
    // --------------------------------------------------------------------------
    uint16_t comfortColor = COLOR_NEON_MINT;
    const char* comfortName = "IDEAL";
    if (telem.humidity > 70.0f) {
        comfortColor = COLOR_NEON_PURPLE;
        comfortName = "HUMID";
    } else if (telem.humidity < 30.0f) {
        comfortColor = COLOR_NEON_AMBER;
        comfortName = "DRY";
    } else if (telem.temperature > 28.0f) {
        comfortColor = COLOR_NEON_CORAL;
        comfortName = "WARM";
    } else if (telem.temperature < 19.0f) {
        comfortColor = COLOR_NEON_BLUE;
        comfortName = "COOL";
    }

    gfx.drawCard(6, STATUS_BAR_HEIGHT + 76, 116, 20, COLOR_SLATE_MID, comfortColor);
    gfx.fillCircle(15, STATUS_BAR_HEIGHT + 86, 3, comfortColor);
    gfx.drawText(22, STATUS_BAR_HEIGHT + 82, comfortName, comfortColor, 1);

    char heatBuf[16];
    snprintf(heatBuf, sizeof(heatBuf), "HI: %.1fC", telem.heatIndex);
    gfx.drawTextRight(116, STATUS_BAR_HEIGHT + 82, heatBuf, COLOR_TEXT_LIGHT, 1);

    // --------------------------------------------------------------------------
    // 3. Bottom Row: 24-Sample Temperature Trend Card (y = 112..138, height 26)
    // --------------------------------------------------------------------------
    gfx.drawCard(6, STATUS_BAR_HEIGHT + 98, 116, 26, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(11, STATUS_BAR_HEIGHT + 101, "TREND", COLOR_TEXT_MUTED, 1);

    // Auto-scale min and max
    float minT = _tempHistory[0];
    float maxT = _tempHistory[0];
    for (int i = 1; i < CLIMATE_HISTORY_POINTS; ++i) {
        if (_tempHistory[i] < minT) minT = _tempHistory[i];
        if (_tempHistory[i] > maxT) maxT = _tempHistory[i];
    }
    if (maxT - minT < 2.0f) {
        minT -= 1.0f;
        maxT += 1.0f;
    }

    char rangeBuf[16];
    snprintf(rangeBuf, sizeof(rangeBuf), "%.0f-%.0fC", minT, maxT);
    gfx.drawTextRight(116, STATUS_BAR_HEIGHT + 101, rangeBuf, COLOR_TEXT_MUTED, 1);

    gfx.drawSparkline(11, STATUS_BAR_HEIGHT + 110, 106, 11, _tempHistory, CLIMATE_HISTORY_POINTS, minT, maxT, COLOR_NEON_CYAN);

    // --------------------------------------------------------------------------
    // 4. Bottom Navigation Hint (y = 148, 10px gap above)
    // --------------------------------------------------------------------------
    gfx.drawTextCentered(64, SCREEN_HEIGHT - 12, "1s: Back   3s: Home", COLOR_TEXT_MUTED, 1);
}

bool ClimateApp::onButtonEvent(const ButtonEvent& evt) {
    return false; // Standard navigation
}
