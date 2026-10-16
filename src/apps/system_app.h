#pragma once
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — System Settings & Hardware Telemetry App
// ==============================================================================

class SystemApp : public AppBase {
public:
    SystemApp() = default;
    ~SystemApp() override = default;

    AppMetadata getMetadata() const override {
        return { "system", "System", 11, false };
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;

private:
    uint8_t _brightness = 80;
};
