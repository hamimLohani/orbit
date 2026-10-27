#include "src/apps/astro_app.h"
#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/display/theme.h"
#include <cmath>

REGISTER_APP(AstroApp);

void AstroApp::onInit() {}
void AstroApp::onEnter() {}
void AstroApp::onExit() {}

void AstroApp::onUpdate(uint32_t deltaMs) {
    _starRot += 0.2f * (deltaMs / 1000.0f);
    if (_starRot > 6.283f) _starRot -= 6.283f;

    uint32_t now = millis();
    if (now - _lastTick > 1000) {
        _lastTick = now;
        if (_issPassSeconds > 0) _issPassSeconds--;
        else _issPassSeconds = 5400; // Reset to 90 min orbit
    }
}

void AstroApp::onRender(RenderEngine& gfx) {
    TelemetryData telem = SharedData::getInstance().getData();

    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);
    gfx.drawHeader("ASTRO DASHBOARD", 5);

    int16_t cx = 36;
    int16_t cy = STATUS_BAR_HEIGHT + 45; // y = 59
    int16_t radius = 18; // Spans y=39..79 with glow, cleanly below header line at y=37

    // Draw Lunar Graphic
    renderMoon(gfx, cx, cy, radius, telem.moonPhase);

    // Lunar Phase Details Card (Right: starts at x=64, y=40, width=56, height=40)
    const char* phaseName = "New Moon";
    float p = telem.moonPhase;
    int illum = (int)(fabs(p - 0.0f) <= 0.5f ? (p * 200.0f) : ((1.0f - p) * 200.0f));

    if (p < 0.05f || p > 0.95f) phaseName = "New Moon";
    else if (p < 0.23f) phaseName = "Waxing Crescent";
    else if (p < 0.27f) phaseName = "First Quarter";
    else if (p < 0.48f) phaseName = "Waxing Gibbous";
    else if (p < 0.52f) phaseName = "Full Moon";
    else if (p < 0.73f) phaseName = "Waning Gibbous";
    else if (p < 0.77f) phaseName = "Last Quarter";
    else phaseName = "Waning Crescent";

    gfx.drawCard(64, STATUS_BAR_HEIGHT + 26, 56, 40, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(70, STATUS_BAR_HEIGHT + 31, "LUNAR", COLOR_TEXT_MUTED, 1);
    char illumBuf[16];
    snprintf(illumBuf, sizeof(illumBuf), "%d%%", illum);
    gfx.drawText(70, STATUS_BAR_HEIGHT + 43, illumBuf, COLOR_TEXT_LIGHT, 2);

    // Phase Name beneath
    gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 71, phaseName, COLOR_NEON_CYAN, 1);

    // ISS Live Space Station Pass Card (starts at y=96, height 36 -> ends at y=132)
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 82, 112, 36, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 87, "ISS NEXT PASS", COLOR_TEXT_MUTED, 1);

    uint32_t issMins = _issPassSeconds / 60;
    uint32_t issSecs = _issPassSeconds % 60;
    char issBuf[24];
    snprintf(issBuf, sizeof(issBuf), "Pass in %02lum %02lus", issMins, issSecs);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 99, issBuf, COLOR_NEON_MINT, 1);

    gfx.drawTextCentered(64, SCREEN_HEIGHT - 10, "ORBIT ASTRAL HUD", COLOR_TEXT_MUTED, 1);
}

void AstroApp::renderMoon(RenderEngine& gfx, int16_t cx, int16_t cy, int16_t radius, float phase) {
    // Dark lunar base circle
    gfx.fillCircle(cx, cy, radius, COLOR_SLATE_DARK);
    gfx.drawCircle(cx, cy, radius, COLOR_SLATE_BORDER);

    // Render illuminated hemisphere
    for (int16_t y = -radius; y <= radius; ++y) {
        int16_t xSpan = (int16_t)sqrt((radius * radius) - (y * y));
        if (phase >= 0.5f) {
            // Waning: illuminated on left
            gfx.drawFastHLine(cx - xSpan, cy + y, xSpan, COLOR_WHITE);
        } else {
            // Waxing: illuminated on right
            gfx.drawFastHLine(cx, cy + y, xSpan, COLOR_WHITE);
        }
    }

    // Outer glow ring
    gfx.drawCircle(cx, cy, radius + 2, COLOR_SLATE_MID);
}

bool AstroApp::onButtonEvent(const ButtonEvent& evt) {
    return false;
}
