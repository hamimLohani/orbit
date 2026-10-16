#include "src/apps/system_app.h"
#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/core/settings_manager.h"
#include "src/display/bitmap_loader.h"
#include "src/display/theme.h"

REGISTER_APP(SystemApp);

void SystemApp::onInit() {
    _brightness = SettingsManager::getInstance().getBrightness();
}

void SystemApp::onEnter() {
    _brightness = SettingsManager::getInstance().getBrightness();
}

void SystemApp::onExit() {}
void SystemApp::onUpdate(uint32_t deltaMs) {}

void SystemApp::onRender(RenderEngine& gfx) {
    TelemetryData telem = SharedData::getInstance().getData();

    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);
    gfx.drawHeader("SYSTEM SETTINGS", 11);

    // Display Backlight Card
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 28, 112, 36, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 33, "BACKLIGHT", COLOR_TEXT_MUTED, 1);

    char bBuf[16];
    snprintf(bBuf, sizeof(bBuf), "%d%%", _brightness);
    gfx.drawTextRight(112, STATUS_BAR_HEIGHT + 33, bBuf, COLOR_TEXT_LIGHT, 1);
    gfx.drawProgressBar(14, STATUS_BAR_HEIGHT + 47, 100, 6, _brightness, COLOR_NEON_CYAN);

    // Hardware Memory & Uptime Card
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 68, 112, 54, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 73, "HARDWARE STATS", COLOR_TEXT_MUTED, 1);

    char heapBuf[32];
    snprintf(heapBuf, sizeof(heapBuf), "Free Heap: %lu KB", (unsigned long)(ESP.getFreeHeap() / 1024));
    gfx.drawText(14, STATUS_BAR_HEIGHT + 85, heapBuf, COLOR_TEXT_LIGHT, 1);

    char upBuf[32];
    uint32_t up = millis() / 1000;
    snprintf(upBuf, sizeof(upBuf), "Uptime: %luh %lum %lus", up / 3600, (up % 3600) / 60, up % 60);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 97, upBuf, COLOR_TEXT_LIGHT, 1);

    char verBuf[32];
    snprintf(verBuf, sizeof(verBuf), "OS v%s", ORBIT_VERSION_STR);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 109, verBuf, COLOR_NEON_MINT, 1);

    gfx.drawTextCentered(64, SCREEN_HEIGHT - 12, "L/DN:- | R/UP:+ Light", COLOR_TEXT_MUTED, 1);
}

bool SystemApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK || evt.type == BUTTON_REPEAT) {
        if (evt.button == BTN_ID_UP || evt.button == BTN_ID_RIGHT) {
            _brightness = (_brightness <= 90) ? (_brightness + 10) : 100;
            RenderEngine::getInstance().setBacklight(_brightness);
            SettingsManager::getInstance().setBrightness(_brightness);
            return true;
        } else if (evt.button == BTN_ID_DOWN || evt.button == BTN_ID_LEFT) {
            _brightness = (_brightness >= 10) ? (_brightness - 10) : 0;
            RenderEngine::getInstance().setBacklight(_brightness);
            SettingsManager::getInstance().setBrightness(_brightness);
            return true;
        }
    }
    return false;
}
