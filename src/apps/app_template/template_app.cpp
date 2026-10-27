#include "src/apps/app_template/template_app.h"
#include "src/core/app_registry.h"
#include "src/display/theme.h"

// Uncomment the line below to register this app with ORBIT:
// REGISTER_APP(TemplateApp);

void TemplateApp::onInit() {
    // Perform one-time hardware or memory setup here
}

void TemplateApp::onEnter() {
    _counter = 0;
}

void TemplateApp::onExit() {
    // Cleanup when leaving app
}

void TemplateApp::onUpdate(uint32_t deltaMs) {
    // Periodic animation or math updates
}

void TemplateApp::onRender(RenderEngine& gfx) {
    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);
    gfx.drawHeader("TEMPLATE APP", 11);

    gfx.drawCard(8, STATUS_BAR_HEIGHT + 26, 112, 64, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 38, "Value Counter", COLOR_TEXT_MUTED, 1);

    char valBuf[16];
    snprintf(valBuf, sizeof(valBuf), "%d", _counter);
    gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 58, valBuf, COLOR_NEON_CYAN, 2);

    gfx.drawTextCentered(64, SCREEN_HEIGHT - 10, "UP / DOWN: Adjust", COLOR_TEXT_MUTED, 1);
}

bool TemplateApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK) {
        if (evt.button == BTN_ID_UP) {
            _counter++;
            return true; // Consumed
        } else if (evt.button == BTN_ID_DOWN) {
            _counter--;
            return true; // Consumed
        }
    }
    return false; // Let OS handle BACK to return home
}

void TemplateApp::onEvent(EventType type, const void* payload) {
    // React to system events if needed
}

void TemplateApp::onWsMessage(const String& cmd, const JsonVariantConst& data) {
    if (cmd == "set_counter") {
        _counter = data["val"] | 0;
    }
}
