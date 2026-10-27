#pragma once
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — Smart Pomodoro Focus & Noise Sentinel App
// ==============================================================================

enum PomodoroState {
    POMO_IDLE = 0,
    POMO_FOCUS,
    POMO_BREAK,
    POMO_PAUSED,
    POMO_EDIT
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

    void setFocusMinutes(uint16_t mins);
    uint16_t getFocusMinutes() const { return _focusMins; }
    void setBreakMinutes(uint16_t mins);
    uint16_t getBreakMinutes() const { return _breakMins; }

private:
    void renderMainTimer(RenderEngine& gfx);
    void renderEditTimer(RenderEngine& gfx);
    void applyPreset(uint8_t index);

    PomodoroState _state = POMO_IDLE;
    uint16_t _focusMins = 25;
    uint16_t _breakMins = 5;
    uint8_t _selectedField = 0; // 0 = Focus Duration, 1 = Break Duration, 2 = Presets
    uint8_t _presetIndex = 0;   // 0 = 25m Pomo, 1 = 15m Sprint, 2 = 45m Deep, 3 = 60m Power

    uint32_t _remainingSeconds = 1500; // 25 mins
    uint32_t _totalSeconds = 1500;
    uint32_t _lastTick = 0;
    uint32_t _noiseViolationMs = 0;
    bool _distractionAlert = false;
};
