#include "src/input/button_manager.h"

void ButtonManager::init() {
    // pin, isDown, lastRawReading, lastDebounceTime, pressTime, lastRepeatTime, hold1sFired, hold2sFired
    _buttons[BTN_ID_UP]    = { PIN_BTN_UP,    false, false, 0, 0, 0, false, false };
    _buttons[BTN_ID_DOWN]  = { PIN_BTN_DOWN,  false, false, 0, 0, 0, false, false };
    _buttons[BTN_ID_LEFT]  = { PIN_BTN_LEFT,  false, false, 0, 0, 0, false, false };
    _buttons[BTN_ID_RIGHT] = { PIN_BTN_RIGHT, false, false, 0, 0, 0, false, false };
    _buttons[BTN_ID_ENTER] = { PIN_BTN_ENTER, false, false, 0, 0, 0, false, false };

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
        bool rawReading = (digitalRead(_buttons[i].pin) == LOW); // Active LOW with pull-up

        // 1. Raw contact change tracking (Software Debounce Filter)
        if (rawReading != _buttons[i].lastRawReading) {
            _buttons[i].lastRawReading = rawReading;
            _buttons[i].lastDebounceTime = now;
        }

        // 2. State transition check after debouncing interval
        if ((now - _buttons[i].lastDebounceTime) >= BTN_DEBOUNCE_MS) {
            if (rawReading != _buttons[i].isDown) {
                _buttons[i].isDown = rawReading;

                if (_buttons[i].isDown) {
                    // === TRANSITION TO PRESSED ===
                    _buttons[i].pressTime = now;
                    _buttons[i].lastRepeatTime = now;
                    _buttons[i].hold1sFired = false;
                    _buttons[i].hold2sFired = false;

                    // Directional buttons (UP, DOWN, LEFT, RIGHT) fire BUTTON_CLICK immediately on press!
                    // This guarantees zero-latency responsiveness for games (Snake, Lander) and navigation.
                    if (i != BTN_ID_ENTER) {
                        if (_callback) {
                            _callback({ static_cast<ButtonId>(i), BUTTON_CLICK, 0 });
                        }
                    }
                } else {
                    // === TRANSITION TO RELEASED ===
                    uint32_t duration = now - _buttons[i].pressTime;

                    if (i == BTN_ID_ENTER) {
                        // Short tap on ENTER (< 1000ms): fires BUTTON_CLICK on clean release
                        if (!_buttons[i].hold1sFired && !_buttons[i].hold2sFired) {
                            if (_callback) {
                                _callback({ BTN_ID_ENTER, BUTTON_CLICK, duration });
                            }
                        } else {
                            if (_callback) {
                                _callback({ BTN_ID_ENTER, BUTTON_RELEASE, duration });
                            }
                        }
                    } else {
                        // Directional buttons release event
                        if (_callback) {
                            _callback({ static_cast<ButtonId>(i), BUTTON_RELEASE, duration });
                        }
                    }

                    _buttons[i].hold1sFired = false;
                    _buttons[i].hold2sFired = false;
                }
            }
        }

        // 3. Continuous Hold Processing (while button remains held down)
        if (_buttons[i].isDown) {
            uint32_t duration = now - _buttons[i].pressTime;

            if (i == BTN_ID_ENTER) {
                // Threshold A: 2.5s Long Hold -> Home
                if (duration >= BTN_HOLD_2S_MS && !_buttons[i].hold2sFired) {
                    _buttons[i].hold2sFired = true;
                    if (_callback) {
                        _callback({ BTN_ID_ENTER, BUTTON_HOLD_2S, duration });
                    }
                }
                // Threshold B: 1.0s Hold -> Back ("1s enter=back")
                else if (duration >= BTN_HOLD_1S_MS && !_buttons[i].hold1sFired) {
                    _buttons[i].hold1sFired = true;
                    if (_callback) {
                        _callback({ BTN_ID_ENTER, BUTTON_HOLD_1S, duration });
                    }
                }
            } else {
                // Directional button auto-repeat (UP, DOWN, LEFT, RIGHT)
                if (duration >= BTN_REPEAT_DELAY_MS) {
                    if ((now - _buttons[i].lastRepeatTime) >= BTN_REPEAT_MS) {
                        _buttons[i].lastRepeatTime = now;
                        if (_callback) {
                            _callback({ static_cast<ButtonId>(i), BUTTON_REPEAT, duration });
                        }
                    }
                }
            }
        }
    }
}
