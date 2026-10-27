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
        case 5: renderPage5(gfx); break;
        case 6: renderPage6(gfx); break;
        case 7: renderPage7(gfx); break;
        default: renderPage0(gfx); break;
    }

    renderFooter(gfx);
}

// ------------------------------------------------------------------------------
// Page 0: OS Platform & Display Backlight
// ------------------------------------------------------------------------------
// Page 0: OS Platform & Display Backlight
// ------------------------------------------------------------------------------
void SystemApp::renderPage0(RenderEngine& gfx) {
    gfx.drawHeader("SYSTEM (1/8)", 10);

    // Card 1: Backlight Brightness (y=40, height=46 -> ends at 86)
    gfx.drawCard(8, 40, 112, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, 44, "BACKLIGHT", COLOR_TEXT_MUTED, 1);
    char bBuf[16];
    snprintf(bBuf, sizeof(bBuf), "%d%%", _brightness);
    gfx.drawTextRight(112, 44, bBuf, COLOR_NEON_CYAN, 1);
    gfx.drawProgressBar(14, 56, 100, 5, _brightness, COLOR_NEON_CYAN);
    gfx.drawTextCentered(64, 70, "UP/DN: Adjust Dim", COLOR_TEXT_MUTED, 1);

    // Card 2: OS Platform (y=90, height=46 -> ends at 136)
    gfx.drawCard(8, 90, 112, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, 94, "ORBIT OS PLATFORM", COLOR_TEXT_MUTED, 1);
    char verBuf[24];
    snprintf(verBuf, sizeof(verBuf), "OS: v%s", ORBIT_VERSION_STR);
    gfx.drawText(14, 105, verBuf, COLOR_NEON_MINT, 1);
    char idfBuf[24];
    snprintf(idfBuf, sizeof(idfBuf), "IDF: %s", esp_get_idf_version());
    gfx.drawText(14, 115, idfBuf, COLOR_TEXT_LIGHT, 1);
    char upBuf[24];
    uint32_t up = millis() / 1000;
    snprintf(upBuf, sizeof(upBuf), "Up: %02luh %02lum %02lus", up / 3600, (up % 3600) / 60, up % 60);
    gfx.drawText(14, 125, upBuf, COLOR_TEXT_MUTED, 1);
}

// ------------------------------------------------------------------------------
// Page 1: Processor Silicon & Architecture
// ------------------------------------------------------------------------------
void SystemApp::renderPage1(RenderEngine& gfx) {
    gfx.drawHeader("PROCESSOR (2/8)", 10);

    // Card 1: CPU Architecture (y=40, height=46 -> ends at 86)
    gfx.drawCard(8, 40, 112, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, 44, "PROCESSOR SILICON", COLOR_TEXT_MUTED, 1);
    char chipBuf[24];
    snprintf(chipBuf, sizeof(chipBuf), "MCU: %s", ESP.getChipModel());
    gfx.drawText(14, 55, chipBuf, COLOR_NEON_CYAN, 1);
    char freqBuf[24];
    snprintf(freqBuf, sizeof(freqBuf), "Clock: %d MHz", (int)ESP.getCpuFreqMHz());
    gfx.drawText(14, 65, freqBuf, COLOR_TEXT_LIGHT, 1);
    char coreBuf[24];
    snprintf(coreBuf, sizeof(coreBuf), "%d Cores (Xtensa)", (int)ESP.getChipCores());
    gfx.drawText(14, 75, coreBuf, COLOR_TEXT_MUTED, 1);

    // Card 2: Features & Revision (y=90, height=46 -> ends at 136)
    gfx.drawCard(8, 90, 112, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, 94, "SILICON ARCHITECTURE", COLOR_TEXT_MUTED, 1);
    char revBuf[24];
    snprintf(revBuf, sizeof(revBuf), "Rev: v%d | LX7", (int)ESP.getChipRevision());
    gfx.drawText(14, 105, revBuf, COLOR_NEON_MINT, 1);
    gfx.drawText(14, 115, "SIMD: PIE Vector", COLOR_TEXT_LIGHT, 1);
    gfx.drawText(14, 125, "Crypto: AES / SHA", COLOR_TEXT_MUTED, 1);
}

// ------------------------------------------------------------------------------
// Page 2: Device ID & MAC Addresses
// ------------------------------------------------------------------------------
void SystemApp::renderPage2(RenderEngine& gfx) {
    gfx.drawHeader("DEVICE ID (3/8)", 10);

    // Card 1: MAC Identity (y=40, height=46 -> ends at 86)
    gfx.drawCard(8, 40, 112, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, 44, "STATION MAC ADDR", COLOR_TEXT_MUTED, 1);
    String mac = WiFi.macAddress();
    gfx.drawText(14, 56, mac.c_str(), COLOR_NEON_MINT, 1);
    gfx.drawText(14, 69, "I/F: 802.11 b/g/n", COLOR_TEXT_MUTED, 1);

    // Card 2: eFuse Hardware UID (y=90, height=46 -> ends at 136)
    gfx.drawCard(8, 90, 112, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, 94, "UNIQUE HARDWARE UID", COLOR_TEXT_MUTED, 1);
    uint64_t efuse = ESP.getEfuseMac();
    char efuseBuf[24];
    snprintf(efuseBuf, sizeof(efuseBuf), "0x%08llX", efuse & 0xFFFFFFFFULL);
    gfx.drawText(14, 107, efuseBuf, COLOR_NEON_CYAN, 1);
    gfx.drawText(14, 121, "Target: ESP32-S3", COLOR_TEXT_MUTED, 1);
}

// ------------------------------------------------------------------------------
// Page 3: Internal SRAM Heap (Hero Card)
// ------------------------------------------------------------------------------
void SystemApp::renderPage3(RenderEngine& gfx) {
    gfx.drawHeader("SRAM HEAP (4/8)", 10);

    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totHeap = ESP.getHeapSize();
    uint8_t heapFreePct = (totHeap > 0) ? (uint8_t)(((uint64_t)freeHeap * 100) / totHeap) : 0;
    uint32_t minHeap = ESP.getMinFreeHeap();
    uint32_t maxAlloc = ESP.getMaxAllocHeap();

    // Single large, beautifully spaced hero card (y=40, height=96 -> ends at 136)
    gfx.drawCard(8, 40, 112, 96, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, 45, "INTERNAL SRAM HEAP", COLOR_TEXT_MUTED, 1);

    char heapBuf[24];
    snprintf(heapBuf, sizeof(heapBuf), "Free: %luK (%d%%)", (unsigned long)(freeHeap / 1024), (int)heapFreePct);
    gfx.drawText(14, 56, heapBuf, COLOR_NEON_CYAN, 1);

    // Visual Progress Bar for Free Space
    gfx.drawProgressBar(14, 68, 100, 5, heapFreePct, COLOR_NEON_CYAN);

    char totBuf[24];
    snprintf(totBuf, sizeof(totBuf), "Total:   %lu KB", (unsigned long)(totHeap / 1024));
    gfx.drawText(14, 79, totBuf, COLOR_TEXT_LIGHT, 1);

    char minBuf[24];
    snprintf(minBuf, sizeof(minBuf), "MinFree: %lu KB", (unsigned long)(minHeap / 1024));
    gfx.drawText(14, 90, minBuf, COLOR_TEXT_LIGHT, 1);

    char maxBuf[24];
    snprintf(maxBuf, sizeof(maxBuf), "MaxBlock:%lu KB", (unsigned long)(maxAlloc / 1024));
    gfx.drawText(14, 101, maxBuf, COLOR_TEXT_MUTED, 1);

    gfx.drawText(14, 116, "Type: Fast SRAM", COLOR_NEON_MINT, 1);
}

// ------------------------------------------------------------------------------
// Page 4: External PSRAM (Hero Card)
// ------------------------------------------------------------------------------
void SystemApp::renderPage4(RenderEngine& gfx) {
    gfx.drawHeader("PSRAM (5/8)", 10);

    gfx.drawCard(8, 40, 112, 96, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, 45, "EXTERNAL PSRAM", COLOR_TEXT_MUTED, 1);

    if (psramFound()) {
        uint32_t psramTot = ESP.getPsramSize();
        uint32_t psramFree = ESP.getFreePsram();
        uint8_t psramFreePct = (psramTot > 0) ? (uint8_t)(((uint64_t)psramFree * 100) / psramTot) : 0;

        char psramBuf[24];
        snprintf(psramBuf, sizeof(psramBuf), "Free: %luK (%d%%)", (unsigned long)(psramFree / 1024), (int)psramFreePct);
        gfx.drawText(14, 56, psramBuf, COLOR_NEON_MINT, 1);

        gfx.drawProgressBar(14, 68, 100, 5, psramFreePct, COLOR_NEON_MINT);

        char totBuf[24];
        snprintf(totBuf, sizeof(totBuf), "Total:   %lu MB", (unsigned long)(psramTot / (1024 * 1024)));
        gfx.drawText(14, 79, totBuf, COLOR_TEXT_LIGHT, 1);

        gfx.drawText(14, 90, "Mode:    Octal SPI", COLOR_TEXT_LIGHT, 1);
        gfx.drawText(14, 101, "Speed:   80 MHz DDR", COLOR_TEXT_MUTED, 1);
        gfx.drawText(14, 116, "Status:  ACTIVE (OPI)", COLOR_NEON_MINT, 1);
    } else {
        gfx.drawText(14, 62, "Status: Not Active", COLOR_NEON_RED, 1);
        gfx.drawText(14, 78, "Operating on", COLOR_TEXT_MUTED, 1);
        gfx.drawText(14, 91, "Internal SRAM only", COLOR_TEXT_MUTED, 1);
        gfx.drawText(14, 112, "OPI PSRAM required", COLOR_NEON_AMBER, 1);
    }
}

// ------------------------------------------------------------------------------
// Page 5: SPI Flash ROM & Firmware Partition (Hero Card)
// ------------------------------------------------------------------------------
void SystemApp::renderPage5(RenderEngine& gfx) {
    gfx.drawHeader("FLASH ROM (6/8)", 10);

    uint32_t flashMb = ESP.getFlashChipSize() / (1024 * 1024);
    uint32_t flashMhz = ESP.getFlashChipSpeed() / 1000000;
    uint32_t sketchKb = ESP.getSketchSize() / 1024;
    uint32_t freeSketchKb = ESP.getFreeSketchSpace() / 1024;
    uint32_t totSketchKb = sketchKb + freeSketchKb;
    uint8_t sketchPct = (totSketchKb > 0) ? (uint8_t)((sketchKb * 100) / totSketchKb) : 0;
    uint8_t freeAppPct = (totSketchKb > 0) ? (100 - sketchPct) : 0;

    gfx.drawCard(8, 40, 112, 96, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, 45, "SPI FLASH & ROM", COLOR_TEXT_MUTED, 1);

    char flashBuf[24];
    snprintf(flashBuf, sizeof(flashBuf), "Flash: %luMB @%luM", (unsigned long)flashMb, (unsigned long)flashMhz);
    gfx.drawText(14, 56, flashBuf, COLOR_NEON_CYAN, 1);

    char sketchBuf[24];
    snprintf(sketchBuf, sizeof(sketchBuf), "App: %lu KB (%d%%)", (unsigned long)sketchKb, (int)sketchPct);
    gfx.drawText(14, 68, sketchBuf, COLOR_TEXT_LIGHT, 1);

    gfx.drawProgressBar(14, 79, 100, 5, sketchPct, COLOR_NEON_CYAN);

    char freeAppBuf[24];
    snprintf(freeAppBuf, sizeof(freeAppBuf), "Free: %luK (%d%%)", (unsigned long)freeSketchKb, (int)freeAppPct);
    gfx.drawText(14, 91, freeAppBuf, COLOR_NEON_MINT, 1);

    gfx.drawText(14, 104, "Mode: QIO Quad SPI", COLOR_TEXT_LIGHT, 1);
    gfx.drawText(14, 117, "Part: Default 8MB", COLOR_TEXT_MUTED, 1);
}

// ------------------------------------------------------------------------------
// Page 6: Storage: LittleFS & MicroSD
// ------------------------------------------------------------------------------
void SystemApp::renderPage6(RenderEngine& gfx) {
    gfx.drawHeader("STORAGE (7/8)", 10);

    // Card 1: LittleFS Internal Flash Storage (y=40, height=46 -> ends at 86)
    gfx.drawCard(8, 40, 112, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, 44, "LITTLEFS PARTITION", COLOR_TEXT_MUTED, 1);

    uint32_t lfsUsed = LittleFS.usedBytes() / 1024;
    uint32_t lfsTot = LittleFS.totalBytes() / 1024;
    uint32_t lfsFree = (lfsTot >= lfsUsed) ? (lfsTot - lfsUsed) : 0;
    uint8_t lfsFreePct = (lfsTot > 0) ? (uint8_t)((lfsFree * 100) / lfsTot) : 0;

    char lfsBuf[24];
    snprintf(lfsBuf, sizeof(lfsBuf), "Free: %luK (%d%%)", (unsigned long)lfsFree, (int)lfsFreePct);
    gfx.drawText(14, 55, lfsBuf, COLOR_NEON_CYAN, 1);

    gfx.drawProgressBar(14, 66, 100, 4, lfsFreePct, COLOR_NEON_CYAN);

    char lfsTotBuf[24];
    snprintf(lfsTotBuf, sizeof(lfsTotBuf), "Total: %lu KB (FS)", (unsigned long)lfsTot);
    gfx.drawText(14, 74, lfsTotBuf, COLOR_TEXT_MUTED, 1);

    // Card 2: MicroSD Card (y=90, height=46 -> ends at 136)
    gfx.drawCard(8, 90, 112, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, 94, "MICROSD CARD (SPI)", COLOR_TEXT_MUTED, 1);

    if (BitmapLoader::getInstance().isSdMounted()) {
        uint32_t totalMb = 0, usedMb = 0;
        BitmapLoader::getInstance().getStorageStats(totalMb, usedMb);
        uint32_t freeMb = (totalMb >= usedMb) ? (totalMb - usedMb) : 0;
        uint8_t freePct = (totalMb > 0) ? (uint8_t)(((uint64_t)freeMb * 100) / totalMb) : 0;

        char sdUsedBuf[24];
        snprintf(sdUsedBuf, sizeof(sdUsedBuf), "Free: %d%% (%luMB)", (int)freePct, (unsigned long)freeMb);
        gfx.drawText(14, 105, sdUsedBuf, COLOR_NEON_MINT, 1);

        gfx.drawProgressBar(14, 116, 100, 4, freePct, COLOR_NEON_MINT);

        char sdTotBuf[24];
        snprintf(sdTotBuf, sizeof(sdTotBuf), "Total: %lu MB (FAT)", (unsigned long)totalMb);
        gfx.drawText(14, 125, sdTotBuf, COLOR_TEXT_MUTED, 1);
    } else {
        gfx.drawText(14, 108, "Card: Not Mounted", COLOR_NEON_AMBER, 1);
        gfx.drawText(14, 122, "Slot: GPIO 47 CS", COLOR_TEXT_MUTED, 1);
    }
}

// ------------------------------------------------------------------------------
// Page 7: Network: Wi-Fi, IP & Gateway
// ------------------------------------------------------------------------------
void SystemApp::renderPage7(RenderEngine& gfx) {
    gfx.drawHeader("NETWORK (8/8)", 10);

    TelemetryData telem = SharedData::getInstance().getData();

    // Card 1: Wi-Fi Radio (y=40, height=46 -> ends at 86)
    gfx.drawCard(8, 40, 112, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, 44, "WI-FI RADIO STATUS", COLOR_TEXT_MUTED, 1);

    if (telem.wifiConnected) {
        String ssid = WifiManager::getInstance().getSsid();
        if (ssid.length() > 14) ssid = ssid.substring(0, 12) + "..";
        char ssidBuf[24];
        snprintf(ssidBuf, sizeof(ssidBuf), "STA: %s", ssid.c_str());
        gfx.drawText(14, 55, ssidBuf, COLOR_NEON_MINT, 1);

        char rssiBuf[24];
        snprintf(rssiBuf, sizeof(rssiBuf), "Signal: %d dBm", telem.wifiRssi);
        gfx.drawText(14, 67, rssiBuf, COLOR_TEXT_LIGHT, 1);
    } else {
        gfx.drawText(14, 55, "AP: ORBIT-Setup", COLOR_NEON_PURPLE, 1);
        gfx.drawText(14, 67, "Hotspot Active", COLOR_TEXT_MUTED, 1);
    }

    // Card 2: IP Configuration (y=90, height=46 -> ends at 136)
    gfx.drawCard(8, 90, 112, 46, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(14, 94, "TCP/IP CONFIGURATION", COLOR_TEXT_MUTED, 1);

    char ipBuf[24];
    snprintf(ipBuf, sizeof(ipBuf), "IP: %s", telem.ipAddress);
    gfx.drawText(14, 105, ipBuf, COLOR_NEON_CYAN, 1);

    if (telem.wifiConnected) {
        char gwBuf[24];
        snprintf(gwBuf, sizeof(gwBuf), "GW: %s", WiFi.gatewayIP().toString().c_str());
        gfx.drawText(14, 116, gwBuf, COLOR_TEXT_LIGHT, 1);
        gfx.drawText(14, 126, "Port: 80/81 (WS)", COLOR_TEXT_MUTED, 1);
    } else {
        char hostBuf[24];
        snprintf(hostBuf, sizeof(hostBuf), "Host:%s.local", MDNS_HOSTNAME);
        gfx.drawText(14, 116, hostBuf, COLOR_TEXT_LIGHT, 1);
        gfx.drawText(14, 126, "AP: 192.168.4.1", COLOR_TEXT_MUTED, 1);
    }
}

// ------------------------------------------------------------------------------
// Bottom Navigation Footer
// ------------------------------------------------------------------------------
void SystemApp::renderFooter(RenderEngine& gfx) {
    char pageBuf[24];
    snprintf(pageBuf, sizeof(pageBuf), "< Page %d of %d >", (int)_currentPage + 1, (int)_maxPages);
    gfx.drawTextCentered(64, 147, pageBuf, COLOR_TEXT_MUTED, 1);
}

bool SystemApp::onButtonEvent(const ButtonEvent& evt) {
    // 1s Enter = 1 step back
    if (evt.button == BTN_ID_ENTER && (evt.type == BUTTON_HOLD_1S || evt.type == BUTTON_HOLD_05S)) {
        if (_currentPage > 0) {
            _currentPage = 0; // 1 step back to Page 0
            RenderEngine::getInstance().showToast("BACK", "Page 1 (Overview)", COLOR_TEXT_MUTED, 800);
            return true;
        }
        return false; // On Page 0: allow AppRegistry to step back out to Home
    }

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

        // LEFT / RIGHT / ENTER navigation cycles through diagnostic pages
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
