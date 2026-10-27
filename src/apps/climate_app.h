#pragma once
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — Climate & Environment App (DHT11)
// ==============================================================================

#define CLIMATE_HISTORY_POINTS 24

class ClimateApp : public AppBase {
public:
    ClimateApp() = default;
    ~ClimateApp() override = default;

    AppMetadata getMetadata() const override {
        return { "climate", "Climate", 2, true }; // Background logging enabled
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;

private:
    float _tempHistory[CLIMATE_HISTORY_POINTS] = {0};
    uint32_t _lastHistorySample = 0;
};
