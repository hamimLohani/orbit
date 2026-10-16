#pragma once
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — Home Dashboard App
// ==============================================================================

class HomeApp : public AppBase {
public:
    HomeApp() = default;
    ~HomeApp() override = default;

    AppMetadata getMetadata() const override {
        return { "home", "Home", 0, false };
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;

private:
    float _orbitAngle1 = 0.0f;
    float _orbitAngle2 = 1.57f;
    float _orbitAngle3 = 3.14f;
};
