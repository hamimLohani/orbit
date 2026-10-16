#include "src/apps/lights_app.h"
#include "src/core/app_registry.h"
#include "src/lighting/rgb_controller.h"
#include "src/display/theme.h"

REGISTER_APP(LightsApp);

void LightsApp::onInit() {}
void LightsApp::onEnter() {}
void LightsApp::onExit() {}
void LightsApp::onUpdate(uint32_t deltaMs) {}

void LightsApp::onRender(RenderEngine& gfx) {
    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);
    gfx.drawHeader("RGB LIGHTING", 8);

    RgbLightingMode mode = RgbController::getInstance().getMode();
    uint8_t brightness = RgbController::getInstance().getBrightness();

    // Mode Display Card
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 30, 112, 50, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 36, "CURRENT MODE", COLOR_TEXT_MUTED, 1);

    const char* modeName = "AMBIENT";
    uint16_t modeColor = COLOR_NEON_CYAN;
    switch (mode) {
        case RGB_OFF:     modeName = "OFF"; modeColor = COLOR_TEXT_MUTED; break;
        case RGB_AMBIENT: modeName = "AMBIENT CLIMATE"; modeColor = COLOR_NEON_MINT; break;
        case RGB_DISCO:   modeName = "DISCO (SOUND)"; modeColor = COLOR_NEON_AMBER; break;
        case RGB_BREATHE: modeName = "PULSE BREATHE"; modeColor = COLOR_NEON_PURPLE; break;
        case RGB_RAINBOW: modeName = "RAINBOW CYCLE"; modeColor = COLOR_NEON_CYAN; break;
        case RGB_SOLID:   modeName = "CUSTOM SOLID"; modeColor = COLOR_WHITE; break;
        case RGB_ALERT:   modeName = "CRITICAL ALERT"; modeColor = COLOR_NEON_RED; break;
        default: break;
    }

    gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 54, modeName, modeColor, 1);

    // Brightness Card
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 86, 112, 38, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 92, "BRIGHTNESS", COLOR_TEXT_MUTED, 1);

    char bBuf[16];
    uint8_t pct = (brightness * 100) / 255;
    snprintf(bBuf, sizeof(bBuf), "%d%%", pct);
    gfx.drawTextRight(112, STATUS_BAR_HEIGHT + 92, bBuf, COLOR_TEXT_LIGHT, 1);
    gfx.drawProgressBar(14, STATUS_BAR_HEIGHT + 108, 100, 6, pct, COLOR_NEON_CYAN);

    gfx.drawTextCentered(64, SCREEN_HEIGHT - 12, "ENTER: Mode | L/R: Dim", COLOR_TEXT_MUTED, 1);
}

bool LightsApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK || evt.type == BUTTON_REPEAT) {
        if (evt.button == BTN_ID_ENTER) {
            uint8_t nextMode = (RgbController::getInstance().getMode() + 1) % RGB_MODE_MAX;
            RgbController::getInstance().setMode(static_cast<RgbLightingMode>(nextMode));
            return true;
        } else if (evt.button == BTN_ID_UP || evt.button == BTN_ID_RIGHT) {
            uint8_t b = RgbController::getInstance().getBrightness();
            b = (b <= 230) ? (b + 25) : 255;
            RgbController::getInstance().setBrightness(b);
            return true;
        } else if (evt.button == BTN_ID_DOWN || evt.button == BTN_ID_LEFT) {
            uint8_t b = RgbController::getInstance().getBrightness();
            b = (b >= 25) ? (b - 25) : 0;
            RgbController::getInstance().setBrightness(b);
            return true;
        }
    }
    return false;
}
