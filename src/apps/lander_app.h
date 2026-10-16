#pragma once
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — Orbit Lunar Lander Retro Physics Game
// ==============================================================================

class LanderApp : public AppBase {
public:
    LanderApp() = default;
    ~LanderApp() override = default;

    AppMetadata getMetadata() const override {
        return { "lander", "Lander", 9, false };
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;

private:
    void resetGame();

    float _posX = 64.0f;
    float _posY = 35.0f;
    float _velX = 0.0f;
    float _velY = 0.0f;
    float _fuel = 100.0f;
    bool _thrusting = false;
    bool _gameOver = false;
    bool _landed = false;
    int _score = 0;
};
