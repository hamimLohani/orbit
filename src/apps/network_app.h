#pragma once
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — Network & Wi-Fi Management App
// ==============================================================================

class NetworkApp : public AppBase {
public:
    NetworkApp() = default;
    ~NetworkApp() override = default;

    AppMetadata getMetadata() const override {
        return { "network", "Network", 10, false };
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;
};
