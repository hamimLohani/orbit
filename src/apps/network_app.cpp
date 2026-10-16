#include "src/apps/network_app.h"
#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/network/wifi_manager.h"
#include "src/display/theme.h"

REGISTER_APP(NetworkApp);

void NetworkApp::onInit() {}
void NetworkApp::onEnter() {}
void NetworkApp::onExit() {}
void NetworkApp::onUpdate(uint32_t deltaMs) {}

void NetworkApp::onRender(RenderEngine& gfx) {
    TelemetryData telem = SharedData::getInstance().getData();

    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);
    gfx.drawHeader("NETWORK STATUS", 10);

    // Wi-Fi Status Card
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 28, 112, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);

    if (telem.wifiConnected) {
        gfx.drawText(14, STATUS_BAR_HEIGHT + 33, "WI-FI CONNECTED", COLOR_NEON_MINT, 1);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 45, telem.ipAddress, COLOR_TEXT_LIGHT, 1);

        char rssiBuf[24];
        snprintf(rssiBuf, sizeof(rssiBuf), "Signal: %d dBm", telem.wifiRssi);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 57, rssiBuf, COLOR_TEXT_MUTED, 1);
    } else {
        gfx.drawText(14, STATUS_BAR_HEIGHT + 33, "DISCONNECTED", COLOR_NEON_RED, 1);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 48, "AP: ORBIT-Setup", COLOR_TEXT_LIGHT, 1);
    }

    // mDNS & Web Dashboard URL Card
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 78, 112, 44, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 83, "WEB DASHBOARD", COLOR_TEXT_MUTED, 1);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 95, "http://orbit.local", COLOR_NEON_CYAN, 1);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 107, "Port 80 /ws", COLOR_TEXT_MUTED, 1);

    gfx.drawTextCentered(64, SCREEN_HEIGHT - 12, "ENTER: Reconnect", COLOR_TEXT_MUTED, 1);
}

bool NetworkApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK && (evt.button == BTN_ID_ENTER || evt.button == BTN_ID_SELECT)) {
        WifiManager::getInstance().init();
        RenderEngine::getInstance().showToast("NETWORK", "Reconnecting Wi-Fi...", COLOR_NEON_CYAN, 3000);
        return true;
    }
    return false;
}
