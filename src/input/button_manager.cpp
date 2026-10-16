#include "src/input/button_manager.h"

void ButtonManager::init() {
    _buttons[BTN_ID_UP]     = { PIN_BTN_UP,     false, false, false, 0, 0 };
    _buttons[BTN_ID_DOWN]   = { PIN_BTN_DOWN,   false, false, false, 0, 0 };
    _buttons[BTN_ID_SELECT] = { PIN_BTN_SELECT, false, false, false, 0, 0 };
    _buttons[BTN_ID_BACK]   = { PIN_BTN_BACK,   false, false, false, 0, 0 };
    _buttons[BTN_ID_ACTION] = { PIN_BTN_ACTION, false, false, false, 0, 0 };

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

        // State change detection with debounce
        if (rawPressed && !_buttons[i].wasDown) {
            _buttons[i].isDown = true;
            _buttons[i].wasDown = true;
            _buttons[i].holdFired = false;
            _buttons[i].pressTime = now;
            _buttons[i].lastRepeatTime = now;
        } else if (!rawPressed && _buttons[i].wasDown) {
            uint32_t duration = now - _buttons[i].pressTime;
            if (duration >= BTN_DEBOUNCE_MS) {
                if (!_buttons[i].holdFired) {
                    // Short Click
                    if (_callback) {
                        _callback({ static_cast<ButtonId>(i), BUTTON_CLICK, duration });
                    }
                } else {
                    // Released after hold
                    if (_callback) {
                        _callback({ static_cast<ButtonId>(i), BUTTON_RELEASE, duration });
                    }
                }
            }
            _buttons[i].isDown = false;
            _buttons[i].wasDown = false;
            _buttons[i].holdFired = false;
        } else if (rawPressed && _buttons[i].isDown) {
            uint32_t duration = now - _buttons[i].pressTime;
            if (!_buttons[i].holdFired && duration >= BTN_HOLD_MS) {
                _buttons[i].holdFired = true;
                _buttons[i].lastRepeatTime = now;
                if (_callback) {
                    _callback({ static_cast<ButtonId>(i), BUTTON_HOLD, duration });
                }
            } else if (_buttons[i].holdFired && (now - _buttons[i].lastRepeatTime >= BTN_REPEAT_MS)) {
                _buttons[i].lastRepeatTime = now;
                if (_callback) {
                    _callback({ static_cast<ButtonId>(i), BUTTON_REPEAT, duration });
                }
            }
        }
    }
}
