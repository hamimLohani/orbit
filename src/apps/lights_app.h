#pragma once
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — Ambient & Mood Lighting App
// ==============================================================================

class LightsApp : public AppBase {
public:
    LightsApp() = default;
    ~LightsApp() override = default;

    AppMetadata getMetadata() const override {
        return { "lights", "Lights", 8, false };
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;
};
