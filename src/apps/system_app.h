#pragma once
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — System Settings & Comprehensive Hardware Diagnostics App
// Displays Storage, Device ID, IP, Flash, ROM, PSRAM, OS Version, and Hardware Info
// ==============================================================================

class SystemApp : public AppBase {
public:
    SystemApp() = default;
    ~SystemApp() override = default;

    AppMetadata getMetadata() const override {
        return { "system", "System", 10, false };
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;

    uint8_t getCurrentPage() const { return _currentPage; }
    void setPage(uint8_t page) { _currentPage = (page < _maxPages) ? page : 0; }

private:
    void renderPage0(RenderEngine& gfx); // OS Platform & Display Backlight
    void renderPage1(RenderEngine& gfx); // Processor Silicon & Architecture
    void renderPage2(RenderEngine& gfx); // Device ID & MAC Addresses
    void renderPage3(RenderEngine& gfx); // Internal SRAM Heap
    void renderPage4(RenderEngine& gfx); // External PSRAM
    void renderPage5(RenderEngine& gfx); // SPI Flash ROM & Partition
    void renderPage6(RenderEngine& gfx); // Storage: LittleFS & MicroSD
    void renderPage7(RenderEngine& gfx); // Network: Wi-Fi, IP & Gateway
    void renderFooter(RenderEngine& gfx);

    uint8_t _brightness = 80;
    uint8_t _currentPage = 0;
    const uint8_t _maxPages = 8;
};
