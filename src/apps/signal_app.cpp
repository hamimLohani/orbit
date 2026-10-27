#include "src/apps/signal_app.h"
#include "src/core/app_registry.h"
#include "src/lighting/indicator_led.h"
#include "src/lighting/rgb_controller.h"
#include "src/display/theme.h"

REGISTER_APP(SignalApp);

void SignalApp::onInit() {}
void SignalApp::onEnter() {}
void SignalApp::onExit() {
    if (_strobeActive) {
        _strobeActive = false;
        IndicatorLed::getInstance().setPattern(LED_HEARTBEAT);
    }
}

void SignalApp::onUpdate(uint32_t deltaMs) {}

void SignalApp::onRender(RenderEngine& gfx) {
    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);
    gfx.drawHeader("SIGNAL / ALERTS", 7);

    // Alert Status Card (starts at y=40, height 48 -> ends at y=88)
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 26, 112, 48, COLOR_SLATE_MID, _strobeActive ? COLOR_NEON_RED : COLOR_SLATE_BORDER);

    if (_strobeActive) {
        gfx.fillCircle(20, STATUS_BAR_HEIGHT + 40, 4, COLOR_NEON_RED);
        gfx.drawText(30, STATUS_BAR_HEIGHT + 36, "BEACON ACTIVE", COLOR_NEON_RED, 1);
        gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 54, "SOS Flashing...", COLOR_WHITE, 1);
    } else {
        gfx.drawIcon(16, STATUS_BAR_HEIGHT + 33, 7, COLOR_NEON_CYAN, COLOR_SLATE_MID);
        gfx.drawText(38, STATUS_BAR_HEIGHT + 37, "SYSTEM READY", COLOR_TEXT_LIGHT, 1);
        gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 54, "Webhooks Active", COLOR_TEXT_MUTED, 1);
    }

    // Ping / Test Card (starts at y=94, height 36 -> ends at y=130)
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 80, 112, 36, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 86, "TRIGGER BEACON", COLOR_TEXT_MUTED, 1);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 98, "Press ENTER", COLOR_NEON_AMBER, 1);

    // Bottom hint (20 chars = 120px, fits cleanly within 128px screen)
    gfx.drawTextCentered(64, SCREEN_HEIGHT - 10, "ENTER: Toggle Beacon", COLOR_TEXT_MUTED, 1);
}

bool SignalApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK && (evt.button == BTN_ID_ENTER || evt.button == BTN_ID_SELECT)) {
        _strobeActive = !_strobeActive;
        if (_strobeActive) {
            IndicatorLed::getInstance().setPattern(LED_BLINK_FAST);
            RgbController::getInstance().setMode(RGB_ALERT);
            RenderEngine::getInstance().showToast("ALERT BEACON", "Beacon Strobe Triggered", COLOR_NEON_RED, 3000);
        } else {
            IndicatorLed::getInstance().setPattern(LED_OFF);
            RgbController::getInstance().setMode(RGB_OFF);
        }
        return true;
    }
    return false;
}
