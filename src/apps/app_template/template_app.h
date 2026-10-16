#pragma once
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — Boilerplate App Template
// Copy this template to create any new application in minutes!
// ==============================================================================

class TemplateApp : public AppBase {
public:
    TemplateApp() = default;
    ~TemplateApp() override = default;

    // App Metadata for the OS launcher
    AppMetadata getMetadata() const override {
        return {
            "template",     // Unique slug ID
            "Template",     // Display Title
            11,             // Icon index in assets/icons.h
            false           // backgroundRunning flag
        };
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;

    void onEvent(EventType type, const void* payload) override;
    void onWsMessage(const String& cmd, const JsonVariantConst& data) override;

private:
    int _counter = 0;
};
