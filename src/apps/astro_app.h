#pragma once
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — Space & Astro Telemetry Dashboard
// ==============================================================================

class AstroApp : public AppBase {
public:
    AstroApp() = default;
    ~AstroApp() override = default;

    AppMetadata getMetadata() const override {
        return { "astro", "Astro", 5, false };
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;

private:
    void renderMoon(RenderEngine& gfx, int16_t cx, int16_t cy, int16_t radius, float phase);

    float _starRot = 0.0f;
    uint32_t _issPassSeconds = 4820; // Simulated countdown to next pass
    uint32_t _lastTick = 0;
};
