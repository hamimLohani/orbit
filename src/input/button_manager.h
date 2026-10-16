#pragma once
#include <Arduino.h>
#include "include/config.h"

// ==============================================================================
// ORBIT v2 — 5-Button Input Manager (UP, DOWN, LEFT, RIGHT, ENTER)
// ENTER short press = Select/Enter
// ENTER 0.5s hold   = Back
// ENTER 2.0s hold   = Home
// ==============================================================================

enum ButtonId {
    BTN_ID_UP = 0,
    BTN_ID_DOWN,
    BTN_ID_LEFT,
    BTN_ID_RIGHT,
    BTN_ID_ENTER,
    BTN_ID_COUNT
};

// Aliases for compatibility
#define BTN_ID_SELECT BTN_ID_ENTER
#define BTN_ID_BACK   BTN_ID_LEFT
#define BTN_ID_ACTION BTN_ID_ENTER

enum ButtonEventType {
    BUTTON_CLICK = 0,    // Short click (< 500ms)
    BUTTON_HOLD_05S,     // Held >= 500ms (Back)
    BUTTON_HOLD_2S,      // Held >= 2000ms (Home)
    BUTTON_RELEASE,
    BUTTON_REPEAT
};

// Backward compatibility alias
#define BUTTON_HOLD BUTTON_HOLD_05S

struct ButtonEvent {
    ButtonId button;
    ButtonEventType type;
    uint32_t durationMs;
};

typedef std::function<void(const ButtonEvent&)> ButtonCallback;

class ButtonManager {
public:
    static ButtonManager& getInstance() {
        static ButtonManager instance;
        return instance;
    }

    void init();
    void update(); // Called from Core 1 loop

    void setCallback(ButtonCallback cb);
    bool isPressed(ButtonId btn) const;

private:
    ButtonManager() = default;
    ~ButtonManager() = default;

    struct ButtonState {
        uint8_t pin;
        bool isDown;
        bool wasDown;
        bool hold05Fired;
        bool hold2sFired;
        uint32_t pressTime;
        uint32_t lastRepeatTime;
    };

    ButtonState _buttons[BTN_ID_COUNT];
    ButtonCallback _callback = nullptr;
};
