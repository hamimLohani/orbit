#pragma once
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — Aura Interactive Companion & Emotion Engine
// ==============================================================================

enum AuraMood {
    MOOD_HAPPY = 0,
    MOOD_CURIOUS,
    MOOD_SURPRISED,
    MOOD_SLEEPY,
    MOOD_COLD,
    MOOD_HOT,
    MOOD_DISCO
};

class CompanionApp : public AppBase {
public:
    CompanionApp() = default;
    ~CompanionApp() override = default;

    AppMetadata getMetadata() const override {
        return { "aura", "Aura", 1, true };
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;
    void onEvent(EventType type, const void* payload) override;

private:
    void renderEyes(RenderEngine& gfx, int16_t cx, int16_t cy);

    AuraMood _mood = MOOD_HAPPY;
    int16_t _eyeOffsetX = 0;
    int16_t _eyeOffsetY = 0;
    bool _isBlinking = false;
    uint32_t _blinkTimer = 0;
    uint32_t _nextBlinkInterval = 3000;
    uint32_t _lookTimer = 0;
    float _sleepBubbleY = 0.0f;
    int _petCounter = 0;
};
