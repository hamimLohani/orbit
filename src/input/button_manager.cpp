#include "src/input/button_manager.h"

void ButtonManager::init() {
    _buttons[BTN_ID_UP]    = { PIN_BTN_UP,    false, false, false, false, 0, 0 };
    _buttons[BTN_ID_DOWN]  = { PIN_BTN_DOWN,  false, false, false, false, 0, 0 };
    _buttons[BTN_ID_LEFT]  = { PIN_BTN_LEFT,  false, false, false, false, 0, 0 };
    _buttons[BTN_ID_RIGHT] = { PIN_BTN_RIGHT, false, false, false, false, 0, 0 };
    _buttons[BTN_ID_ENTER] = { PIN_BTN_ENTER, false, false, false, false, 0, 0 };

    for (int i = 0; i < BTN_ID_COUNT; ++i) {
        pinMode(_buttons[i].pin, INPUT_PULLUP);
    }
}

void ButtonManager::setCallback(ButtonCallback cb) {
    _callback = cb;
}

bool ButtonManager::isPressed(ButtonId btn) const {
    if (btn >= 0 && btn < BTN_ID_COUNT) {
        return _buttons[btn].isDown;
    }
    return false;
}

void ButtonManager::update() {
    uint32_t now = millis();

    for (int i = 0; i < BTN_ID_COUNT; ++i) {
        bool rawPressed = (digitalRead(_buttons[i].pin) == LOW); // Active LOW

        // 1. Initial Press (Debounced)
        if (rawPressed && !_buttons[i].wasDown) {
            _buttons[i].isDown = true;
            _buttons[i].wasDown = true;
            _buttons[i].hold05Fired = false;
            _buttons[i].hold2sFired = false;
            _buttons[i].pressTime = now;
            _buttons[i].lastRepeatTime = now;
        } 
        // 2. Button Released
        else if (!rawPressed && _buttons[i].wasDown) {
            uint32_t duration = now - _buttons[i].pressTime;
            if (duration >= BTN_DEBOUNCE_MS) {
                // If neither 0.5s nor 2s hold fired, this is a clean short tap (< 500ms)
                if (!_buttons[i].hold05Fired && !_buttons[i].hold2sFired) {
                    if (_callback) {
                        _callback({ static_cast<ButtonId>(i), BUTTON_CLICK, duration });
                    }
                } else {
                    if (_callback) {
                        _callback({ static_cast<ButtonId>(i), BUTTON_RELEASE, duration });
                    }
                }
            }
            _buttons[i].isDown = false;
            _buttons[i].wasDown = false;
            _buttons[i].hold05Fired = false;
            _buttons[i].hold2sFired = false;
        } 
        // 3. Button Still Held Down
        else if (rawPressed && _buttons[i].isDown) {
            uint32_t duration = now - _buttons[i].pressTime;

            // Threshold A: 2.0s Hold (Home)
            if (duration >= BTN_HOLD_2S_MS && !_buttons[i].hold2sFired) {
                _buttons[i].hold2sFired = true;
                if (_callback) {
                    _callback({ static_cast<ButtonId>(i), BUTTON_HOLD_2S, duration });
                }
            }
            // Threshold B: 0.5s Hold (Back)
            else if (duration >= BTN_HOLD_05S_MS && !_buttons[i].hold05Fired) {
                _buttons[i].hold05Fired = true;
                _buttons[i].lastRepeatTime = now;
                if (_callback) {
                    _callback({ static_cast<ButtonId>(i), BUTTON_HOLD_05S, duration });
                }
            }
            // Directional key repeat (UP, DOWN, LEFT, RIGHT)
            else if (_buttons[i].hold05Fired && (i != BTN_ID_ENTER) && (now - _buttons[i].lastRepeatTime >= BTN_REPEAT_MS)) {
                _buttons[i].lastRepeatTime = now;
                if (_callback) {
                    _callback({ static_cast<ButtonId>(i), BUTTON_REPEAT, duration });
                }
            }
        }
    }
}
