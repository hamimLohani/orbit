#pragma once
#include <Arduino.h>
#include "include/config.h"

// ==============================================================================
// ORBIT v2 — 5-Button Non-Blocking Input Manager
// ==============================================================================

enum ButtonId {
    BTN_ID_UP = 0,
    BTN_ID_DOWN,
    BTN_ID_SELECT,
    BTN_ID_BACK,
    BTN_ID_ACTION,
    BTN_ID_COUNT
};

enum ButtonEventType {
    BUTTON_CLICK = 0,
    BUTTON_HOLD,
    BUTTON_RELEASE,
    BUTTON_REPEAT
};

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
    void update(); // Called frequently from loop() on Core 1

    void setCallback(ButtonCallback cb);
    bool isPressed(ButtonId btn) const;

private:
    ButtonManager() = default;
    ~ButtonManager() = default;

    struct ButtonState {
        uint8_t pin;
        bool isDown;
        bool wasDown;
        bool holdFired;
        uint32_t pressTime;
        uint32_t lastRepeatTime;
    };

    ButtonState _buttons[BTN_ID_COUNT];
    ButtonCallback _callback = nullptr;
};
