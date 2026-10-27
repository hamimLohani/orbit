#include "src/apps/system_app.h"
#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/core/settings_manager.h"
#include "src/display/bitmap_loader.h"
#include "src/display/theme.h"
#include "src/network/wifi_manager.h"
#include <WiFi.h>
#include <LittleFS.h>
#include <esp_system.h>

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
    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);

    switch (_currentPage) {
        case 0: renderPage0(gfx); break;
        case 1: renderPage1(gfx); break;
        case 2: renderPage2(gfx); break;
        case 3: renderPage3(gfx); break;
        case 4: renderPage4(gfx); break;
        default: renderPage0(gfx); break;
    }

    renderFooter(gfx);
}

// ------------------------------------------------------------------------------
// Page 0: Overview, OS & Display Settings
// ------------------------------------------------------------------------------
void SystemApp::renderPage0(RenderEngine& gfx) {
    gfx.drawHeader("SYSTEM (1/5)", 11);

    // Card 1: Backlight Brightness
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 26, 112, 36, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 31, "BACKLIGHT", COLOR_TEXT_MUTED, 1);

    char bBuf[16];
    snprintf(bBuf, sizeof(bBuf), "%d%%", _brightness);
    gfx.drawTextRight(112, STATUS_BAR_HEIGHT + 31, bBuf, COLOR_NEON_CYAN, 1);
    gfx.drawProgressBar(14, STATUS_BAR_HEIGHT + 45, 100, 6, _brightness, COLOR_NEON_CYAN);

    // Card 2: OS & Kernel Stats
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 66, 112, 54, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 71, "ORBIT OS PLATFORM", COLOR_TEXT_MUTED, 1);

    char verBuf[24];
    snprintf(verBuf, sizeof(verBuf), "OS: v%s", ORBIT_VERSION_STR);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 83, verBuf, COLOR_NEON_MINT, 1);

    char idfBuf[24];
    snprintf(idfBuf, sizeof(idfBuf), "IDF: %s", esp_get_idf_version());
    gfx.drawText(14, STATUS_BAR_HEIGHT + 95, idfBuf, COLOR_TEXT_LIGHT, 1);

    char upBuf[24];
    uint32_t up = millis() / 1000;
    snprintf(upBuf, sizeof(upBuf), "Up: %02luh %02lum %02lus", up / 3600, (up % 3600) / 60, up % 60);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 107, upBuf, COLOR_TEXT_MUTED, 1);
}

// ------------------------------------------------------------------------------
// Page 1: Chip Model, Cores, MAC & Device ID
// ------------------------------------------------------------------------------
void SystemApp::renderPage1(RenderEngine& gfx) {
    gfx.drawHeader("DEVICE ID (2/5)", 11);

    // Card 1: Silicon & CPU Architecture
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 26, 112, 48, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 31, "PROCESSOR SILICON", COLOR_TEXT_MUTED, 1);

    char chipBuf[24];
    snprintf(chipBuf, sizeof(chipBuf), "MCU: %s", ESP.getChipModel());
    gfx.drawText(14, STATUS_BAR_HEIGHT + 43, chipBuf, COLOR_NEON_CYAN, 1);

    char coreBuf[24];
    snprintf(coreBuf, sizeof(coreBuf), "%d Cores @ %dMHz", (int)ESP.getChipCores(), (int)ESP.getCpuFreqMHz());
    gfx.drawText(14, STATUS_BAR_HEIGHT + 54, coreBuf, COLOR_TEXT_LIGHT, 1);

    char revBuf[24];
    snprintf(revBuf, sizeof(revBuf), "Rev: v%d | Arch: LX7", (int)ESP.getChipRevision());
    gfx.drawText(14, STATUS_BAR_HEIGHT + 64, revBuf, COLOR_TEXT_MUTED, 1);

    // Card 2: MAC & Unique Hardware Identity
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 78, 112, 44, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 83, "MAC & HARDWARE ID", COLOR_TEXT_MUTED, 1);

    String mac = WiFi.macAddress();
    gfx.drawText(14, STATUS_BAR_HEIGHT + 95, mac.c_str(), COLOR_NEON_MINT, 1);

    uint64_t efuse = ESP.getEfuseMac();
    char efuseBuf[24];
    snprintf(efuseBuf, sizeof(efuseBuf), "UID: 0x%08llX", efuse & 0xFFFFFFFFULL);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 107, efuseBuf, COLOR_TEXT_LIGHT, 1);
}

// ------------------------------------------------------------------------------
// Page 2: Memory Architecture, Heap & PSRAM
// ------------------------------------------------------------------------------
void SystemApp::renderPage2(RenderEngine& gfx) {
    gfx.drawHeader("MEMORY (3/5)", 11);

    // Card 1: Internal SRAM & Heap
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 26, 112, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 31, "INTERNAL SRAM HEAP", COLOR_TEXT_MUTED, 1);

    char heapBuf[24];
    snprintf(heapBuf, sizeof(heapBuf), "Free: %lu KB", (unsigned long)(ESP.getFreeHeap() / 1024));
    gfx.drawText(14, STATUS_BAR_HEIGHT + 43, heapBuf, COLOR_NEON_CYAN, 1);

    char minHeapBuf[24];
    snprintf(minHeapBuf, sizeof(minHeapBuf), "Min Free: %lu KB", (unsigned long)(ESP.getMinFreeHeap() / 1024));
    gfx.drawText(14, STATUS_BAR_HEIGHT + 54, minHeapBuf, COLOR_TEXT_LIGHT, 1);

    char totHeapBuf[24];
    snprintf(totHeapBuf, sizeof(totHeapBuf), "Total: %lu KB", (unsigned long)(ESP.getHeapSize() / 1024));
    gfx.drawText(14, STATUS_BAR_HEIGHT + 64, totHeapBuf, COLOR_TEXT_MUTED, 1);

    // Card 2: External Embedded PSRAM (SPIRAM)
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 76, 112, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 81, "EXTERNAL PSRAM", COLOR_TEXT_MUTED, 1);

    if (psramFound()) {
        char psramBuf[24];
        snprintf(psramBuf, sizeof(psramBuf), "Total: %lu MB (AP)", (unsigned long)(ESP.getPsramSize() / (1024 * 1024)));
        gfx.drawText(14, STATUS_BAR_HEIGHT + 93, psramBuf, COLOR_NEON_MINT, 1);

        char psramFreeBuf[24];
        snprintf(psramFreeBuf, sizeof(psramFreeBuf), "Free: %lu KB", (unsigned long)(ESP.getFreePsram() / 1024));
        gfx.drawText(14, STATUS_BAR_HEIGHT + 104, psramFreeBuf, COLOR_TEXT_LIGHT, 1);
    } else {
        gfx.drawText(14, STATUS_BAR_HEIGHT + 95, "PSRAM: Not Active", COLOR_NEON_RED, 1);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 107, "Using SRAM only", COLOR_TEXT_MUTED, 1);
    }
}

// ------------------------------------------------------------------------------
// Page 3: Storage: Flash ROM, LittleFS & MicroSD
// ------------------------------------------------------------------------------
void SystemApp::renderPage3(RenderEngine& gfx) {
    gfx.drawHeader("STORAGE (4/5)", 11);

    // Card 1: Flash ROM & Firmware Partition
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 26, 112, 48, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 31, "SPI FLASH & LFS", COLOR_TEXT_MUTED, 1);

    uint32_t flashMb = ESP.getFlashChipSize() / (1024 * 1024);
    uint32_t flashMhz = ESP.getFlashChipSpeed() / 1000000;
    char flashBuf[24];
    snprintf(flashBuf, sizeof(flashBuf), "Flash: %luMB @ %luMHz", (unsigned long)flashMb, (unsigned long)flashMhz);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 43, flashBuf, COLOR_NEON_CYAN, 1);

    uint32_t sketchKb = ESP.getSketchSize() / 1024;
    char sketchBuf[24];
    snprintf(sketchBuf, sizeof(sketchBuf), "Sketch: %lu KB", (unsigned long)sketchKb);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 54, sketchBuf, COLOR_TEXT_LIGHT, 1);

    uint32_t lfsUsed = LittleFS.usedBytes() / 1024;
    uint32_t lfsTot = LittleFS.totalBytes() / 1024;
    char lfsBuf[24];
    snprintf(lfsBuf, sizeof(lfsBuf), "LFS: %luK / %luK", (unsigned long)lfsUsed, (unsigned long)lfsTot);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 64, lfsBuf, COLOR_TEXT_MUTED, 1);

    // Card 2: MicroSD Card Storage
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 78, 112, 44, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 83, "MICROSD CARD (SPI)", COLOR_TEXT_MUTED, 1);

    if (BitmapLoader::getInstance().isSdMounted()) {
        uint32_t totalMb = 0, usedMb = 0;
        BitmapLoader::getInstance().getStorageStats(totalMb, usedMb);

        char sdBuf[24];
        snprintf(sdBuf, sizeof(sdBuf), "Size: %lu MB", (unsigned long)totalMb);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 95, sdBuf, COLOR_NEON_MINT, 1);

        char sdUsedBuf[24];
        snprintf(sdUsedBuf, sizeof(sdUsedBuf), "Used: %lu MB", (unsigned long)usedMb);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 107, sdUsedBuf, COLOR_TEXT_LIGHT, 1);
    } else {
        gfx.drawText(14, STATUS_BAR_HEIGHT + 95, "Card: Not Detected", COLOR_NEON_AMBER, 1);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 107, "Slot: GPIO 47 CS", COLOR_TEXT_MUTED, 1);
    }
}

// ------------------------------------------------------------------------------
// Page 4: Network, Wi-Fi, IP & Connectivity
// ------------------------------------------------------------------------------
void SystemApp::renderPage4(RenderEngine& gfx) {
    gfx.drawHeader("NETWORK (5/5)", 11);

    TelemetryData telem = SharedData::getInstance().getData();

    // Card 1: Wi-Fi Radio Status
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 26, 112, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 31, "WI-FI RADIO", COLOR_TEXT_MUTED, 1);

    if (telem.wifiConnected) {
        String ssid = WifiManager::getInstance().getSsid();
        if (ssid.length() > 14) ssid = ssid.substring(0, 12) + "..";
        char ssidBuf[24];
        snprintf(ssidBuf, sizeof(ssidBuf), "STA: %s", ssid.c_str());
        gfx.drawText(14, STATUS_BAR_HEIGHT + 43, ssidBuf, COLOR_NEON_MINT, 1);

        char rssiBuf[24];
        snprintf(rssiBuf, sizeof(rssiBuf), "Signal: %d dBm", telem.wifiRssi);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 54, rssiBuf, COLOR_TEXT_LIGHT, 1);
    } else {
        gfx.drawText(14, STATUS_BAR_HEIGHT + 43, "AP: ORBIT-Setup", COLOR_NEON_PURPLE, 1);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 54, "Hotspot Active", COLOR_TEXT_MUTED, 1);
    }

    // Card 2: IP Configuration & Hostname
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 76, 112, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, STATUS_BAR_HEIGHT + 81, "TCP/IP ADDRESS", COLOR_TEXT_MUTED, 1);

    gfx.drawText(14, STATUS_BAR_HEIGHT + 93, telem.ipAddress, COLOR_NEON_CYAN, 1);

    if (telem.wifiConnected) {
        char gwBuf[24];
        snprintf(gwBuf, sizeof(gwBuf), "GW: %s", WiFi.gatewayIP().toString().c_str());
        gfx.drawText(14, STATUS_BAR_HEIGHT + 104, gwBuf, COLOR_TEXT_LIGHT, 1);
    } else {
        char hostBuf[24];
        snprintf(hostBuf, sizeof(hostBuf), "Host: %s.local", MDNS_HOSTNAME);
        gfx.drawText(14, STATUS_BAR_HEIGHT + 104, hostBuf, COLOR_TEXT_LIGHT, 1);
    }
}

// ------------------------------------------------------------------------------
// Bottom Navigation Footer
// ------------------------------------------------------------------------------
void SystemApp::renderFooter(RenderEngine& gfx) {
    char pageBuf[32];
    snprintf(pageBuf, sizeof(pageBuf), "< L   Page %d/%d   R >", (int)_currentPage + 1, (int)_maxPages);
    gfx.drawTextCentered(64, SCREEN_HEIGHT - 10, pageBuf, COLOR_TEXT_MUTED, 1);
}

bool SystemApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK || evt.type == BUTTON_REPEAT) {
        // On Page 0, UP and DOWN adjust screen backlight
        if (_currentPage == 0 && evt.button == BTN_ID_UP) {
            _brightness = (_brightness <= 90) ? (_brightness + 10) : 100;
            RenderEngine::getInstance().setBacklight(_brightness);
            SettingsManager::getInstance().setBrightness(_brightness);
            return true;
        } else if (_currentPage == 0 && evt.button == BTN_ID_DOWN) {
            _brightness = (_brightness >= 10) ? (_brightness - 10) : 0;
            RenderEngine::getInstance().setBacklight(_brightness);
            SettingsManager::getInstance().setBrightness(_brightness);
            return true;
        }

        // LEFT / RIGHT navigation cycles through diagnostic pages
        if (evt.button == BTN_ID_RIGHT || evt.button == BTN_ID_ENTER) {
            _currentPage = (_currentPage + 1) % _maxPages;
            return true;
        } else if (evt.button == BTN_ID_LEFT) {
            _currentPage = (_currentPage > 0) ? (_currentPage - 1) : (_maxPages - 1);
            return true;
        }
    }
    return false;
}
