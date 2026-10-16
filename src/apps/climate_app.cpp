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
    gfx.drawHeader("CLIMATE MONITOR", 3);

    // Temperature & Humidity Primary Card
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 30, 112, 38, COLOR_SLATE_MID, COLOR_SLATE_BORDER);

    char tBuf[16], hBuf[16];
    snprintf(tBuf, sizeof(tBuf), "%.1f C", telem.temperature);
    snprintf(hBuf, sizeof(hBuf), "%.0f%% RH", telem.humidity);

    gfx.drawText(14, STATUS_BAR_HEIGHT + 36, "TEMP", COLOR_TEXT_MUTED, 1);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 48, tBuf, COLOR_NEON_CYAN, 2);

    gfx.drawText(72, STATUS_BAR_HEIGHT + 36, "HUMIDITY", COLOR_TEXT_MUTED, 1);
    gfx.drawText(72, STATUS_BAR_HEIGHT + 48, hBuf, COLOR_NEON_MINT, 1);

    // Heat Index & Comfort rating
    const char* comfort = "Ideal Comfort";
    if (telem.humidity > 70.0f) comfort = "Humid Air";
    else if (telem.humidity < 30.0f) comfort = "Dry Air";
    else if (telem.temperature > 28.0f) comfort = "Warm Environment";
    else if (telem.temperature < 19.0f) comfort = "Cool Environment";

    char heatBuf[32];
    snprintf(heatBuf, sizeof(heatBuf), "Heat Idx: %.1f C  |  %s", telem.heatIndex, comfort);
    gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 74, heatBuf, COLOR_TEXT_MUTED, 1);

    // Temperature 24-sample Sparkline Card
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 86, 112, 42, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(12, STATUS_BAR_HEIGHT + 90, "TEMP HISTORY", COLOR_TEXT_MUTED, 1);

    // Find min and max for auto-scale
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

    gfx.drawSparkline(14, STATUS_BAR_HEIGHT + 104, 100, 18, _tempHistory, CLIMATE_HISTORY_POINTS, minT, maxT, COLOR_NEON_CYAN);

    // Bottom Navigation Hint
    gfx.drawTextCentered(64, SCREEN_HEIGHT - 12, "BACK: Return Home", COLOR_TEXT_MUTED, 1);
}

bool ClimateApp::onButtonEvent(const ButtonEvent& evt) {
    return false; // Standard navigation
}
