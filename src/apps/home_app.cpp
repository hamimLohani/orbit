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

    // 1. Center Core Coordinates
    int16_t cx = 64;
    int16_t cy = STATUS_BAR_HEIGHT + 30; // y = 44

    // Outer Orbit Ring (radius 24, satellites span y=17..71)
    gfx.drawCircle(cx, cy, 24, COLOR_SLATE_BORDER);
    gfx.drawCircle(cx, cy, 17, COLOR_SLATE_MID);

    // Orbiting Satellites
    int16_t sat1X = cx + (int16_t)(24 * cos(_orbitAngle1));
    int16_t sat1Y = cy + (int16_t)(24 * sin(_orbitAngle1));
    gfx.fillCircle(sat1X, sat1Y, 3, COLOR_NEON_CYAN);

    int16_t sat2X = cx + (int16_t)(17 * cos(_orbitAngle2));
    int16_t sat2Y = cy + (int16_t)(17 * sin(_orbitAngle2));
    gfx.fillCircle(sat2X, sat2Y, 2, COLOR_NEON_PURPLE);

    int16_t sat3X = cx + (int16_t)(24 * cos(_orbitAngle3));
    int16_t sat3Y = cy + (int16_t)(24 * sin(_orbitAngle3));
    gfx.fillCircle(sat3X, sat3Y, 2, COLOR_NEON_MINT);

    // Central Core Pulse
    gfx.fillCircle(cx, cy, 9, COLOR_SLATE_DARK);
    gfx.drawCircle(cx, cy, 9, COLOR_NEON_CYAN);

    // Mood Avatar in Core Center
    if (telem.micDb > 70.0f) {
        gfx.drawTextCentered(cx, cy - 3, "><", COLOR_NEON_AMBER, 1);
    } else if (telem.temperature > 27.0f) {
        gfx.drawTextCentered(cx, cy - 3, "~.~", COLOR_NEON_CORAL, 1);
    } else if (telem.isNightTime) {
        gfx.drawTextCentered(cx, cy - 3, "--", COLOR_NEON_PURPLE, 1);
    } else {
        gfx.drawTextCentered(cx, cy - 3, "^^", COLOR_NEON_CYAN, 1);
    }

    // 2. Large Live Clock & Current Date Display
    char shortTime[6] = "12:00";
    if (strlen(telem.timeStr) >= 5) {
        strncpy(shortTime, telem.timeStr, 5);
        shortTime[5] = '\0';
    }
    // Live Digital Time (Size 2, y=75..89)
    gfx.drawTextCentered(cx, STATUS_BAR_HEIGHT + 61, shortTime, COLOR_TEXT_LIGHT, 2);

    // Live Calendar Date (Size 1, y=93..101)
    uint16_t dateColor = telem.timeSynced ? COLOR_NEON_CYAN : COLOR_TEXT_MUTED;
    gfx.drawTextCentered(cx, STATUS_BAR_HEIGHT + 79, telem.dateStr, dateColor, 1);

    // 3. Climate Telemetry Cards (Left & Right, starts at y=106, height 28 -> ends at y=134)
    // Left: Temp
    char tempBuf[12];
    snprintf(tempBuf, sizeof(tempBuf), "%.1f C", telem.temperature);
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 92, 53, 28, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(12, STATUS_BAR_HEIGHT + 96, "TEMP", COLOR_TEXT_MUTED, 1);
    gfx.drawText(12, STATUS_BAR_HEIGHT + 108, tempBuf, COLOR_NEON_CYAN, 1);

    // Right: Humidity
    char humBuf[12];
    snprintf(humBuf, sizeof(humBuf), "%.0f%%", telem.humidity);
    gfx.drawCard(67, STATUS_BAR_HEIGHT + 92, 53, 28, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(71, STATUS_BAR_HEIGHT + 96, "HUM", COLOR_TEXT_MUTED, 1);
    gfx.drawText(71, STATUS_BAR_HEIGHT + 108, humBuf, COLOR_NEON_MINT, 1);

    // 4. Ambient Noise Level Bar at Bottom (y=150)
    int16_t noisePercent = map((int)telem.micDb, 30, 90, 0, 100);
    noisePercent = constrain(noisePercent, 0, 100);
    uint16_t barColor = (noisePercent > 70) ? COLOR_NEON_RED : (noisePercent > 45 ? COLOR_NEON_AMBER : COLOR_NEON_CYAN);
    gfx.drawProgressBar(8, SCREEN_HEIGHT - 10, SCREEN_WIDTH - 16, 5, noisePercent, barColor, COLOR_SLATE_DARK);
}

bool HomeApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK) {
        if (evt.button == BTN_ID_UP || evt.button == BTN_ID_LEFT) {
            AppRegistry::getInstance().prevApp();
            return true;
        } else if (evt.button == BTN_ID_DOWN || evt.button == BTN_ID_RIGHT) {
            AppRegistry::getInstance().nextApp();
            return true;
        }
    }
    return false;
}
