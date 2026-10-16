#include "src/apps/home_app.h"
#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/display/theme.h"
#include <cmath>

REGISTER_APP(HomeApp);

void HomeApp::onInit() {}
void HomeApp::onEnter() {}
void HomeApp::onExit() {}

void HomeApp::onUpdate(uint32_t deltaMs) {
    float dt = deltaMs / 1000.0f;
    _orbitAngle1 += 1.8f * dt;
    _orbitAngle2 -= 1.2f * dt;
    _orbitAngle3 += 0.8f * dt;

    if (_orbitAngle1 > 6.283f) _orbitAngle1 -= 6.283f;
    if (_orbitAngle2 < -6.283f) _orbitAngle2 += 6.283f;
    if (_orbitAngle3 > 6.283f) _orbitAngle3 -= 6.283f;
}

void HomeApp::onRender(RenderEngine& gfx) {
    TelemetryData telem = SharedData::getInstance().getData();

    // Background
    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);

    // Center Core Coordinates
    int16_t cx = 64;
    int16_t cy = STATUS_BAR_HEIGHT + 44;

    // Outer Orbit Ring
    gfx.drawCircle(cx, cy, 32, COLOR_SLATE_BORDER);
    gfx.drawCircle(cx, cy, 22, COLOR_SLATE_MID);

    // Orbiting Satellites
    int16_t sat1X = cx + (int16_t)(32 * cos(_orbitAngle1));
    int16_t sat1Y = cy + (int16_t)(32 * sin(_orbitAngle1));
    gfx.fillCircle(sat1X, sat1Y, 3, COLOR_NEON_CYAN);

    int16_t sat2X = cx + (int16_t)(22 * cos(_orbitAngle2));
    int16_t sat2Y = cy + (int16_t)(22 * sin(_orbitAngle2));
    gfx.fillCircle(sat2X, sat2Y, 2, COLOR_NEON_PURPLE);

    int16_t sat3X = cx + (int16_t)(32 * cos(_orbitAngle3));
    int16_t sat3Y = cy + (int16_t)(32 * sin(_orbitAngle3));
    gfx.fillCircle(sat3X, sat3Y, 2, COLOR_NEON_MINT);

    // Central Core Pulse
    gfx.fillCircle(cx, cy, 12, COLOR_SLATE_DARK);
    gfx.drawCircle(cx, cy, 12, COLOR_NEON_CYAN);

    // Mood Avatar in Core Center
    const char* mood = "Serene";
    if (telem.micDb > 70.0f) {
        mood = "Lively";
        gfx.drawTextCentered(cx, cy - 3, "><", COLOR_NEON_AMBER, 1);
    } else if (telem.temperature > 27.0f) {
        mood = "Warm";
        gfx.drawTextCentered(cx, cy - 3, "~.~", COLOR_NEON_CORAL, 1);
    } else if (telem.isNightTime) {
        mood = "Sleeping";
        gfx.drawTextCentered(cx, cy - 3, "--", COLOR_NEON_PURPLE, 1);
    } else {
        gfx.drawTextCentered(cx, cy - 3, "^^", COLOR_NEON_CYAN, 1);
    }

    // Mood String below Core
    gfx.drawTextCentered(cx, cy + 38, mood, COLOR_TEXT_MUTED, 1);

    // Climate Telemetry Cards (Left & Right)
    // Left: Temp
    char tempBuf[12];
    snprintf(tempBuf, sizeof(tempBuf), "%.1f C", telem.temperature);
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 96, 52, 26, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(12, STATUS_BAR_HEIGHT + 100, "TEMP", COLOR_TEXT_MUTED, 1);
    gfx.drawText(12, STATUS_BAR_HEIGHT + 110, tempBuf, COLOR_NEON_CYAN, 1);

    // Right: Humidity
    char humBuf[12];
    snprintf(humBuf, sizeof(humBuf), "%.0f%%", telem.humidity);
    gfx.drawCard(68, STATUS_BAR_HEIGHT + 96, 52, 26, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(72, STATUS_BAR_HEIGHT + 100, "HUM", COLOR_TEXT_MUTED, 1);
    gfx.drawText(72, STATUS_BAR_HEIGHT + 110, humBuf, COLOR_NEON_MINT, 1);

    // Ambient Noise Level Bar at Bottom
    int16_t noisePercent = map((int)telem.micDb, 30, 90, 0, 100);
    noisePercent = constrain(noisePercent, 0, 100);
    uint16_t barColor = (noisePercent > 70) ? COLOR_NEON_RED : (noisePercent > 45 ? COLOR_NEON_AMBER : COLOR_NEON_CYAN);
    gfx.drawProgressBar(8, SCREEN_HEIGHT - 12, SCREEN_WIDTH - 16, 5, noisePercent, barColor, COLOR_SLATE_DARK);
}

bool HomeApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK) {
        if (evt.button == BTN_ID_UP || evt.button == BTN_ID_LEFT) {
            AppRegistry::getInstance().prevApp();
            return true;
        } else if (evt.button == BTN_ID_DOWN || evt.button == BTN_ID_RIGHT) {
            AppRegistry::getInstance().nextApp();
            return true;
        } else if (evt.button == BTN_ID_ENTER) {
            AppRegistry::getInstance().toggleCarousel();
            return true;
        }
    }
    return false;
}
