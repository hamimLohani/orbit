#pragma once
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — Voice Memo & Audio Recorder App
// ==============================================================================

class RecorderApp : public AppBase {
public:
    RecorderApp() = default;
    ~RecorderApp() override = default;

    AppMetadata getMetadata() const override {
        return { "recorder", "Voice Memo", 4, false };
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;

private:
    uint32_t _memoCount = 0;
};
