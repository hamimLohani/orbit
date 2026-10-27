#pragma once
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — Smart Pomodoro Focus & Noise Sentinel App
// ==============================================================================

enum PomodoroState {
    POMO_IDLE = 0,
    POMO_FOCUS,
    POMO_BREAK,
    POMO_PAUSED
};

class FocusApp : public AppBase {
public:
    FocusApp() = default;
    ~FocusApp() override = default;

    AppMetadata getMetadata() const override {
        return { "focus", "Focus", 3, true }; // Background timer execution
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;

private:
    PomodoroState _state = POMO_IDLE;
    uint32_t _remainingSeconds = 1500; // 25 mins
    uint32_t _totalSeconds = 1500;
    uint32_t _lastTick = 0;
    uint32_t _noiseViolationMs = 0;
    bool _distractionAlert = false;
};
