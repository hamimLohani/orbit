#pragma once
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — Signal & Notification Center App
// ==============================================================================

class SignalApp : public AppBase {
public:
    SignalApp() = default;
    ~SignalApp() override = default;

    AppMetadata getMetadata() const override {
        return { "signal", "Signal", 6, false };
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;

private:
    bool _strobeActive = false;
    uint32_t _alertCounter = 0;
};
