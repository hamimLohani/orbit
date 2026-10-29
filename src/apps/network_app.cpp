#include "src/apps/network_app.h"
#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/network/wifi_manager.h"
#include "src/network/ntp_sync.h"
#include "src/network/ota_handler.h"
#include "src/display/theme.h"

REGISTER_APP(NetworkApp);

void NetworkApp::onInit() {
    _currentPage = 0;
}

void NetworkApp::onEnter() {}
void NetworkApp::onExit() {}
void NetworkApp::onUpdate(uint32_t deltaMs) {}

void NetworkApp::onRender(RenderEngine& gfx) {
    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);

    if (OtaHandler::getInstance().isUpdating()) {
        renderOtaProgress(gfx);
        return;
    }

    if (_currentPage == 0) {
        renderPage0(gfx);
    } else if (_currentPage == 1) {
        renderPage1(gfx);
    } else {
        renderPage2(gfx);
    }
}

void NetworkApp::renderOtaProgress(RenderEngine& gfx) {
    gfx.drawHeader("UPDATING OS", 9);

    uint8_t pct = OtaHandler::getInstance().getProgressPercent();
    const char* status = OtaHandler::getInstance().getStatusText();

    // Flashing Hero Card (x=6, y=39, w=116, h=98)
    uint16_t border = ((millis() / 250) % 2 == 0) ? COLOR_NEON_CYAN : COLOR_NEON_MINT;
    gfx.drawCard(6, 39, 116, 98, COLOR_SLATE_MID, border);

    gfx.drawTextCentered(64, 46, "FLASHING OVER WI-FI", COLOR_NEON_CYAN, 1);

    char pctBuf[16];
    snprintf(pctBuf, sizeof(pctBuf), "%u%%", pct);
    gfx.drawTextCentered(64, 62, pctBuf, COLOR_TEXT_LIGHT, 2);

    gfx.drawProgressBar(12, 82, 104, 6, pct, COLOR_NEON_MINT);

    gfx.drawTextCentered(64, 96, status, COLOR_NEON_MINT, 1);
    gfx.drawTextCentered(64, 118, "! KEEP POWER ON !", COLOR_NEON_RED, 1);

    gfx.drawTextCentered(64, 147, "Receiving Binary...", COLOR_TEXT_MUTED, 1);
}

void NetworkApp::renderPage0(RenderEngine& gfx) {
    TelemetryData telem = SharedData::getInstance().getData();
    gfx.drawHeader("NETWORK (1/3)", 9);

    if (telem.wifiConnected) {
        // Card 1: Wi-Fi Connection (x=6, y=39, w=116, h=48)
        gfx.drawCard(6, 39, 116, 48, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
        gfx.drawText(12, 44, "ONLINE (STA MODE)", COLOR_NEON_MINT, 1);
        
        String ssid = WifiManager::getInstance().getSsid();
        if (ssid.length() > 14) ssid = ssid.substring(0, 12) + "..";
        gfx.drawText(12, 56, ssid.c_str(), COLOR_TEXT_LIGHT, 1);
        gfx.drawText(12, 68, telem.ipAddress, COLOR_NEON_CYAN, 1);

        // Card 2: Synchronized Time & Date (x=6, y=89, w=116, h=48)
        gfx.drawCard(6, 89, 116, 48, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
        const char* syncStatus = telem.timeSynced ? "NTP SYNCED (UTC)" : "NTP SYNCING...";
        uint16_t statusColor = telem.timeSynced ? COLOR_NEON_MINT : COLOR_NEON_AMBER;
        gfx.drawText(12, 94, syncStatus, statusColor, 1);
        gfx.drawText(12, 105, telem.timeStr, COLOR_TEXT_LIGHT, 2);
        gfx.drawText(12, 123, telem.dateStr, COLOR_NEON_CYAN, 1);

        // Footer at y=147
        gfx.drawTextCentered(64, 147, "OK:AP  Hold:Forget", COLOR_TEXT_MUTED, 1);
    } else {
        // Hotspot Setup Mode Card (x=6, y=39, w=116, h=48)
        gfx.drawCard(6, 39, 116, 48, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
        gfx.drawText(12, 44, "SOFT-AP HOTSPOT", COLOR_NEON_PURPLE, 1);
        gfx.drawText(12, 56, "SSID: ORBIT-Setup", COLOR_TEXT_LIGHT, 1);
        gfx.drawText(12, 68, "IP:   192.168.4.1", COLOR_NEON_CYAN, 1);

        // Instruction Card (x=6, y=89, w=116, h=48)
        gfx.drawCard(6, 89, 116, 48, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
        gfx.drawText(12, 94, "CAPTIVE PORTAL", COLOR_TEXT_MUTED, 1);
        gfx.drawText(12, 106, "Browse 192.168.4.1", COLOR_NEON_MINT, 1);
        if (strlen(DEFAULT_AP_PASS) >= 8) {
            char passBuf[24];
            snprintf(passBuf, sizeof(passBuf), "Pass: %s", DEFAULT_AP_PASS);
            gfx.drawText(12, 118, passBuf, COLOR_TEXT_LIGHT, 1);
        } else {
            gfx.drawText(12, 118, "Pass: (Open/None)", COLOR_TEXT_LIGHT, 1);
        }

        gfx.drawTextCentered(64, 147, "L/R:Page  OK:Connect", COLOR_TEXT_MUTED, 1);
    }
}

void NetworkApp::renderPage1(RenderEngine& gfx) {
    TelemetryData telem = SharedData::getInstance().getData();
    gfx.drawHeader("WI-FI OTA (2/3)", 9);

    // Card 1: Over-The-Air Flasher Status (x=6, y=39, w=116, h=48)
    gfx.drawCard(6, 39, 116, 48, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(12, 44, "OTA SERVICE STATUS", COLOR_TEXT_MUTED, 1);
    if (telem.wifiConnected) {
        gfx.drawText(12, 56, "Service: ONLINE", COLOR_NEON_MINT, 1);
        gfx.drawText(12, 68, "Port: 3232 (Arduino)", COLOR_TEXT_LIGHT, 1);
    } else {
        gfx.drawText(12, 56, "Service: AP READY", COLOR_NEON_PURPLE, 1);
        gfx.drawText(12, 68, "Port: 3232 / 80", COLOR_TEXT_LIGHT, 1);
    }

    // Card 2: Web Flasher & Upload Studio URL (x=6, y=89, w=116, h=48)
    gfx.drawCard(6, 89, 116, 48, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(12, 94, "WEB FLASHER STUDIO", COLOR_TEXT_MUTED, 1);
    gfx.drawText(12, 106, "orbit.local/update", COLOR_NEON_CYAN, 1);
    if (telem.wifiConnected) {
        char ipUrl[32];
        snprintf(ipUrl, sizeof(ipUrl), "%s/update", telem.ipAddress);
        gfx.drawText(12, 118, ipUrl, COLOR_TEXT_MUTED, 1);
    } else {
        gfx.drawText(12, 118, "192.168.4.1/update", COLOR_TEXT_MUTED, 1);
    }

    // Footer at y=147
    gfx.drawTextCentered(64, 147, "< Page 2 of 3 >", COLOR_TEXT_MUTED, 1);
}

void NetworkApp::renderPage2(RenderEngine& gfx) {
    TelemetryData telem = SharedData::getInstance().getData();
    gfx.drawHeader("RESET WI-FI (3/3)", 9);

    // Card 1: Forget Network Warning (x=6, y=39, w=116, h=48)
    gfx.drawCard(6, 39, 116, 48, COLOR_SLATE_MID, COLOR_NEON_RED);
    gfx.drawText(12, 44, "FORGET SAVED WI-FI", COLOR_NEON_RED, 1);
    gfx.drawText(12, 56, "Press OK to erase", COLOR_TEXT_LIGHT, 1);
    gfx.drawText(12, 68, "SSID & Passwords", COLOR_TEXT_MUTED, 1);

    // Card 2: Hotspot Restore Target (x=6, y=89, w=116, h=48)
    gfx.drawCard(6, 89, 116, 48, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(12, 94, "HOTSPOT FALLBACK", COLOR_NEON_PURPLE, 1);
    gfx.drawText(12, 106, "SSID: ORBIT-Setup", COLOR_TEXT_LIGHT, 1);
    gfx.drawText(12, 118, "IP:   192.168.4.1", COLOR_NEON_CYAN, 1);

    // Footer at y=147
    gfx.drawTextCentered(64, 147, "OK:Confirm Forget", COLOR_NEON_AMBER, 1);
}

bool NetworkApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK) {
        if (evt.button == BTN_ID_RIGHT) {
            _currentPage = (_currentPage + 1) % _maxPages;
            return true;
        }
        if (evt.button == BTN_ID_LEFT) {
            _currentPage = (_currentPage == 0) ? (_maxPages - 1) : (_currentPage - 1);
            return true;
        }

        if (evt.button == BTN_ID_ENTER || evt.button == BTN_ID_SELECT) {
            if (_currentPage == 0) {
                if (WifiManager::getInstance().isConnected()) {
                    WifiManager::getInstance().startApFallback();
                    RenderEngine::getInstance().showToast("WI-FI", "Setup AP Started", COLOR_NEON_PURPLE, 2500);
                } else {
                    WifiManager::getInstance().init();
                    RenderEngine::getInstance().showToast("WI-FI", "Attempting Reconnect", COLOR_NEON_CYAN, 2500);
                }
            } else if (_currentPage == 1) {
                RenderEngine::getInstance().showToast("OTA FLASHER", "Open orbit.local/update", COLOR_NEON_CYAN, 3000);
            } else if (_currentPage == 2) {
                WifiManager::getInstance().forgetWifi();
            }
            return true;
        }
    } else if (evt.type == BUTTON_HOLD_1S || evt.type == BUTTON_HOLD_3S) {
        if (evt.button == BTN_ID_ENTER || evt.button == BTN_ID_SELECT) {
            WifiManager::getInstance().forgetWifi();
            return true;
        }
    }
    return false;
}
