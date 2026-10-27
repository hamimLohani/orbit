#pragma once
#include "src/core/app_base.h"
#include "src/lighting/rgb_controller.h"

// ==============================================================================
// ORBIT v2 — Ambient & Mood Lighting App
// ==============================================================================

class LightsApp : public AppBase {
public:
    LightsApp() = default;
    ~LightsApp() override = default;

    AppMetadata getMetadata() const override {
        return { "lights", "Lights", 7, false };
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;

private:
    uint8_t _selectedField = 0; // 0 = Mode & Power, 1 = Brightness, 2 = Red, 3 = Green, 4 = Blue (when RGB_SOLID)
    RgbLightingMode _lastActiveMode = RGB_AMBIENT;
};
