#include "src/apps/network_app.h"
#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/network/wifi_manager.h"
#include "src/network/ntp_sync.h"
#include "src/display/theme.h"

REGISTER_APP(NetworkApp);

void NetworkApp::onInit() {}
void NetworkApp::onEnter() {}
void NetworkApp::onExit() {}
void NetworkApp::onUpdate(uint32_t deltaMs) {}

void NetworkApp::onRender(RenderEngine& gfx) {
    TelemetryData telem = SharedData::getInstance().getData();

    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);
    gfx.drawHeader("NETWORK & TIME", 10);

    if (telem.wifiConnected) {
        // Card 1: Wi-Fi Connection (starts at y=40, height 42 -> ends at y=82)
        gfx.drawCard(8, STATUS_BAR_HEIGHT + 26, 112, 42, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 31, "ONLINE (STA)", COLOR_NEON_MINT, 1);
        
        String ssid = WifiManager::getInstance().getSsid();
        if (ssid.length() > 14) ssid = ssid.substring(0, 12) + "..";
        gfx.drawText(14, STATUS_BAR_HEIGHT + 43, ssid.c_str(), COLOR_TEXT_LIGHT, 1);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 54, telem.ipAddress, COLOR_NEON_CYAN, 1);

        // Card 2: Synchronized Time & Date (starts at y=86, height 46 -> ends at y=132)
        gfx.drawCard(8, STATUS_BAR_HEIGHT + 72, 112, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
        const char* syncStatus = telem.timeSynced ? "NTP SYNCED" : "NTP SYNCING...";
        uint16_t statusColor = telem.timeSynced ? COLOR_NEON_MINT : COLOR_NEON_AMBER;
        gfx.drawText(14, STATUS_BAR_HEIGHT + 76, syncStatus, statusColor, 1);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 88, telem.timeStr, COLOR_TEXT_LIGHT, 2);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 105, telem.dateStr, COLOR_NEON_CYAN, 1);

        // URL & Action Hint at Bottom
        gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 122, "http://orbit.local", COLOR_TEXT_MUTED, 1);
        gfx.drawTextCentered(64, SCREEN_HEIGHT - 10, "ENTER: Setup AP", COLOR_TEXT_MUTED, 1);
    } else {
        // Hotspot Setup Mode Card (starts at y=40, height 52 -> ends at y=92)
        gfx.drawCard(8, STATUS_BAR_HEIGHT + 26, 112, 52, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 31, "SETUP HOTSPOT", COLOR_NEON_PURPLE, 1);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 43, "SSID: ORBIT-Setup", COLOR_TEXT_LIGHT, 1);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 54, "Pass: orbit1234", COLOR_TEXT_MUTED, 1);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 65, "IP: 192.168.4.1", COLOR_NEON_CYAN, 1);

        // Instruction Card (starts at y=96, height 36 -> ends at y=132)
        gfx.drawCard(8, STATUS_BAR_HEIGHT + 82, 112, 36, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 87, "CONNECT WITH PHONE", COLOR_TEXT_MUTED, 1);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 99, "Browse 192.168.4.1", COLOR_NEON_MINT, 1);

        gfx.drawTextCentered(64, SCREEN_HEIGHT - 10, "ENTER: Reconnect STA", COLOR_TEXT_MUTED, 1);
    }
}

bool NetworkApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK && (evt.button == BTN_ID_ENTER || evt.button == BTN_ID_SELECT)) {
        if (WifiManager::getInstance().isConnected()) {
            WifiManager::getInstance().startApFallback();
            RenderEngine::getInstance().showToast("WI-FI", "Setup AP Started", COLOR_NEON_PURPLE, 2500);
        } else {
            WifiManager::getInstance().init();
            RenderEngine::getInstance().showToast("WI-FI", "Attempting Reconnect", COLOR_NEON_CYAN, 2500);
        }
        return true;
    }
    return false;
}
