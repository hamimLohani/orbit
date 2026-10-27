#include "src/apps/focus_app.h"
#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/core/settings_manager.h"
#include "src/lighting/indicator_led.h"
#include "src/display/theme.h"
#include <cmath>

REGISTER_APP(FocusApp);

struct PomodoroPreset {
    const char* name;
    uint16_t focus;
    uint16_t breakTime;
};

static const PomodoroPreset POMO_PRESETS[] = {
    { "25m POMO", 25, 5 },
    { "15m SPRINT", 15, 3 },
    { "45m DEEP", 45, 10 },
    { "60m POWER", 60, 15 }
};
static const uint8_t POMO_PRESET_COUNT = sizeof(POMO_PRESETS) / sizeof(POMO_PRESETS[0]);

void FocusApp::onInit() {
    _focusMins = SettingsManager::getInstance().getPomodoroFocusMins();
    if (_focusMins < 1 || _focusMins > 90) _focusMins = 25;
    _breakMins = SettingsManager::getInstance().getPomodoroBreakMins();
    if (_breakMins < 1 || _breakMins > 30) _breakMins = 5;

    _totalSeconds = _focusMins * 60;
    _remainingSeconds = _totalSeconds;
}

void FocusApp::onEnter() {}
void FocusApp::onExit() {}

void FocusApp::setFocusMinutes(uint16_t mins) {
    if (mins < 1) mins = 1;
    if (mins > 90) mins = 90;
    _focusMins = mins;
    SettingsManager::getInstance().setPomodoroFocusMins(_focusMins);
    if (_state == POMO_IDLE || _state == POMO_EDIT) {
        _totalSeconds = _focusMins * 60;
        _remainingSeconds = _totalSeconds;
    }
}

void FocusApp::setBreakMinutes(uint16_t mins) {
    if (mins < 1) mins = 1;
    if (mins > 30) mins = 30;
    _breakMins = mins;
    SettingsManager::getInstance().setPomodoroBreakMins(_breakMins);
    if (_state == POMO_BREAK) {
        _totalSeconds = _breakMins * 60;
        _remainingSeconds = _totalSeconds;
    }
}

void FocusApp::applyPreset(uint8_t index) {
    if (index >= POMO_PRESET_COUNT) index = 0;
    _presetIndex = index;
    _focusMins = POMO_PRESETS[index].focus;
    _breakMins = POMO_PRESETS[index].breakTime;
    SettingsManager::getInstance().setPomodoroFocusMins(_focusMins);
    SettingsManager::getInstance().setPomodoroBreakMins(_breakMins);
    _totalSeconds = _focusMins * 60;
    _remainingSeconds = _totalSeconds;
}

void FocusApp::onUpdate(uint32_t deltaMs) {
    uint32_t now = millis();

    // 1. Timer countdown tick
    if (_state == POMO_FOCUS || _state == POMO_BREAK) {
        if (now - _lastTick >= 1000) {
            _lastTick = now;
            if (_remainingSeconds > 0) {
                _remainingSeconds--;
            } else {
                // Completed!
                if (_state == POMO_FOCUS) {
                    _state = POMO_BREAK;
                    _totalSeconds = _breakMins * 60;
                    _remainingSeconds = _totalSeconds;
                    char bBuf[32];
                    snprintf(bBuf, sizeof(bBuf), "Time for a %um break", _breakMins);
                    RenderEngine::getInstance().showToast("FOCUS COMPLETE!", bBuf, COLOR_NEON_MINT, 5000);
                    IndicatorLed::getInstance().flashOnce(500);
                } else {
                    _state = POMO_IDLE;
                    _totalSeconds = _focusMins * 60;
                    _remainingSeconds = _totalSeconds;
                    RenderEngine::getInstance().showToast("BREAK OVER", "Ready to focus?", COLOR_NEON_CYAN, 5000);
                }
            }
        }

        // 2. Ambient Noise Sentinel check
        if (_state == POMO_FOCUS) {
            TelemetryData telem = SharedData::getInstance().getData();
            float threshold = SettingsManager::getInstance().getNoiseThreshold();
            if (telem.micDb > threshold) {
                _noiseViolationMs += deltaMs;
                if (_noiseViolationMs > 4000) { // 4s continuous noise
                    _distractionAlert = true;
                    SharedData::getInstance().setNoiseDistraction(true);
                }
            } else {
                _noiseViolationMs = (_noiseViolationMs > deltaMs) ? (_noiseViolationMs - deltaMs) : 0;
                if (_noiseViolationMs == 0) {
                    _distractionAlert = false;
                    SharedData::getInstance().setNoiseDistraction(false);
                }
            }
        }
    }
}

void FocusApp::onRender(RenderEngine& gfx) {
    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);

    if (_state == POMO_EDIT) {
        renderEditTimer(gfx);
    } else {
        renderMainTimer(gfx);
    }
}

void FocusApp::renderMainTimer(RenderEngine& gfx) {
    gfx.drawHeader("FOCUS FLOW", 3);

    int16_t cx = 64;
    int16_t cy = STATUS_BAR_HEIGHT + 36; // y = 50
    int16_t radius = 21; // cleanly below header line at y=37

    // Progress Arc Calculation
    float progress = (_totalSeconds > 0) ? (1.0f - (float)_remainingSeconds / _totalSeconds) : 0.0f;

    // Dial ring
    gfx.drawCircle(cx, cy, radius, COLOR_SLATE_BORDER);
    gfx.drawCircle(cx, cy, radius - 1, COLOR_SLATE_MID);

    // Progress arc dots
    float maxAngle = progress * 6.283f - 1.57f;
    for (float a = -1.57f; a <= maxAngle; a += 0.25f) {
        int16_t px = cx + (int16_t)((radius - 2) * cos(a));
        int16_t py = cy + (int16_t)((radius - 2) * sin(a));
        uint16_t arcColor = (_state == POMO_BREAK) ? COLOR_NEON_MINT : COLOR_NEON_PURPLE;
        gfx.fillCircle(px, py, 1, arcColor);
    }

    // Time text inside ring
    uint32_t mins = _remainingSeconds / 60;
    uint32_t secs = _remainingSeconds % 60;
    char timeBuf[12];
    snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu", mins, secs);
    gfx.drawTextCentered(cx, cy - 6, timeBuf, COLOR_TEXT_LIGHT, 2);

    // State text
    const char* stateText = "IDLE";
    uint16_t stateColor = COLOR_TEXT_MUTED;
    if (_state == POMO_FOCUS) {
        stateText = "DEEP WORK";
        stateColor = COLOR_NEON_CYAN;
    } else if (_state == POMO_BREAK) {
        stateText = "REST BREAK";
        stateColor = COLOR_NEON_MINT;
    } else if (_state == POMO_PAUSED) {
        stateText = "PAUSED";
        stateColor = COLOR_NEON_AMBER;
    }
    gfx.drawTextCentered(cx, cy + 13, stateText, stateColor, 1);

    // Noise Sentinel Status Card (y=76, height=28 -> ends at 104)
    if (_distractionAlert) {
        gfx.drawCard(6, 76, 116, 28, COLOR_SLATE_MID, COLOR_NEON_RED);
        gfx.drawTextCentered(64, 85, "! NOISY ROOM !", COLOR_NEON_RED, 1);
    } else {
        gfx.drawCard(6, 76, 116, 28, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
        TelemetryData telem = SharedData::getInstance().getData();
        char sentinelBuf[24];
        snprintf(sentinelBuf, sizeof(sentinelBuf), "Sentinel: %.0f dB", telem.micDb);
        gfx.drawTextCentered(64, 82, sentinelBuf, COLOR_TEXT_LIGHT, 1);
        gfx.drawTextCentered(64, 92, "Noise Sentinel Active", COLOR_TEXT_MUTED, 1);
    }

    // Timer Duration & Edit Card (y=108, height=34 -> ends at 142)
    gfx.drawCard(6, 108, 116, 34, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    char durBuf[28];
    snprintf(durBuf, sizeof(durBuf), "%um Focus / %um Rest", _focusMins, _breakMins);
    gfx.drawTextCentered(64, 114, durBuf, COLOR_NEON_MINT, 1);
    if (_state == POMO_IDLE) {
        gfx.drawTextCentered(64, 126, "UP/DN: Edit Timer", COLOR_NEON_CYAN, 1);
    } else if (_state == POMO_FOCUS) {
        gfx.drawTextCentered(64, 126, "Focusing in Flow", COLOR_TEXT_MUTED, 1);
    } else if (_state == POMO_BREAK) {
        gfx.drawTextCentered(64, 126, "Rest & Rehydrate", COLOR_NEON_MINT, 1);
    } else {
        gfx.drawTextCentered(64, 126, "Press OK to Resume", COLOR_NEON_AMBER, 1);
    }

    // Controls footer at y=147
    if (_state == POMO_IDLE) {
        gfx.drawTextCentered(64, 147, "OK:Start   L/R:Adjust", COLOR_TEXT_MUTED, 1);
    } else if (_state == POMO_FOCUS) {
        gfx.drawTextCentered(64, 147, "OK:Pause   1s:Reset", COLOR_TEXT_MUTED, 1);
    } else if (_state == POMO_PAUSED) {
        gfx.drawTextCentered(64, 147, "OK:Resume  1s:Reset", COLOR_TEXT_MUTED, 1);
    } else {
        gfx.drawTextCentered(64, 147, "OK:Pause   1s:Skip", COLOR_TEXT_MUTED, 1);
    }
}

void FocusApp::renderEditTimer(RenderEngine& gfx) {
    gfx.drawHeader("EDIT TIMER", 3);

    // Card 0: Focus Duration (y=39, h=35 -> ends at 74)
    uint16_t b0 = (_selectedField == 0) ? COLOR_NEON_CYAN : COLOR_SLATE_BORDER;
    gfx.drawCard(6, 39, 116, 35, COLOR_SLATE_MID, b0);
    const char* t0 = (_selectedField == 0) ? "> FOCUS TIME" : "  FOCUS TIME";
    gfx.drawText(12, 44, t0, (_selectedField == 0) ? COLOR_NEON_CYAN : COLOR_TEXT_MUTED, 1);
    char fBuf[16];
    snprintf(fBuf, sizeof(fBuf), "%um", _focusMins);
    gfx.drawTextRight(116, 44, fBuf, COLOR_NEON_CYAN, 1);
    gfx.drawProgressBar(12, 57, 104, 4, (_focusMins * 100) / 90, COLOR_NEON_CYAN);

    // Card 1: Break Duration (y=77, h=35 -> ends at 112)
    uint16_t b1 = (_selectedField == 1) ? COLOR_NEON_CYAN : COLOR_SLATE_BORDER;
    gfx.drawCard(6, 77, 116, 35, COLOR_SLATE_MID, b1);
    const char* t1 = (_selectedField == 1) ? "> SHORT BREAK" : "  SHORT BREAK";
    gfx.drawText(12, 82, t1, (_selectedField == 1) ? COLOR_NEON_MINT : COLOR_TEXT_MUTED, 1);
    char bkBuf[16];
    snprintf(bkBuf, sizeof(bkBuf), "%um", _breakMins);
    gfx.drawTextRight(116, 82, bkBuf, COLOR_NEON_MINT, 1);
    gfx.drawProgressBar(12, 95, 104, 4, (_breakMins * 100) / 30, COLOR_NEON_MINT);

    // Card 2: Quick Presets (y=115, h=27 -> ends at 142)
    uint16_t b2 = (_selectedField == 2) ? COLOR_NEON_CYAN : COLOR_SLATE_BORDER;
    gfx.drawCard(6, 115, 116, 27, COLOR_SLATE_MID, b2);
    const char* t2 = (_selectedField == 2) ? "> PRESET" : "  PRESET";
    gfx.drawText(12, 122, t2, (_selectedField == 2) ? COLOR_NEON_AMBER : COLOR_TEXT_MUTED, 1);
    gfx.drawTextRight(116, 122, POMO_PRESETS[_presetIndex].name, COLOR_NEON_AMBER, 1);

    // Footer at y=147
    gfx.drawTextCentered(64, 147, "OK:Save   1s:Back", COLOR_TEXT_MUTED, 1);
}

bool FocusApp::onButtonEvent(const ButtonEvent& evt) {
    // 1s Enter = 1 step back / cancel
    if (evt.button == BTN_ID_ENTER && (evt.type == BUTTON_HOLD_1S || evt.type == BUTTON_HOLD_05S)) {
        if (_state == POMO_EDIT) {
            _state = POMO_IDLE;
            _totalSeconds = _focusMins * 60;
            _remainingSeconds = _totalSeconds;
            RenderEngine::getInstance().showToast("SAVED", "Timer Ready", COLOR_NEON_CYAN, 800);
            return true;
        } else if (_state == POMO_FOCUS || _state == POMO_BREAK) {
            _state = POMO_PAUSED;
            RenderEngine::getInstance().showToast("PAUSED", "Focus Paused", COLOR_NEON_AMBER, 800);
            return true;
        } else if (_state == POMO_PAUSED) {
            _state = POMO_IDLE;
            _totalSeconds = _focusMins * 60;
            _remainingSeconds = _totalSeconds;
            RenderEngine::getInstance().showToast("RESET", "Focus Timer Reset", COLOR_TEXT_MUTED, 800);
            return true;
        }
        return false; // In IDLE: allow AppRegistry to step back out to Home Launcher
    }

    if (evt.type == BUTTON_CLICK || evt.type == BUTTON_REPEAT) {
        // --- EDIT MODE BUTTONS ---
        if (_state == POMO_EDIT) {
            if (evt.button == BTN_ID_UP) {
                _selectedField = (_selectedField == 0) ? 2 : (_selectedField - 1);
                return true;
            } else if (evt.button == BTN_ID_DOWN) {
                _selectedField = (_selectedField + 1) % 3;
                return true;
            } else if (evt.button == BTN_ID_LEFT) {
                if (_selectedField == 0) {
                    _focusMins = (_focusMins > 5) ? (_focusMins - 5) : 1;
                    SettingsManager::getInstance().setPomodoroFocusMins(_focusMins);
                    _totalSeconds = _focusMins * 60;
                    _remainingSeconds = _totalSeconds;
                } else if (_selectedField == 1) {
                    _breakMins = (_breakMins > 1) ? (_breakMins - 1) : 1;
                    SettingsManager::getInstance().setPomodoroBreakMins(_breakMins);
                } else if (_selectedField == 2) {
                    _presetIndex = (_presetIndex == 0) ? (POMO_PRESET_COUNT - 1) : (_presetIndex - 1);
                    applyPreset(_presetIndex);
                }
                return true;
            } else if (evt.button == BTN_ID_RIGHT) {
                if (_selectedField == 0) {
                    _focusMins = (_focusMins <= 85) ? (_focusMins + 5) : 90;
                    SettingsManager::getInstance().setPomodoroFocusMins(_focusMins);
                    _totalSeconds = _focusMins * 60;
                    _remainingSeconds = _totalSeconds;
                } else if (_selectedField == 1) {
                    _breakMins = (_breakMins < 30) ? (_breakMins + 1) : 30;
                    SettingsManager::getInstance().setPomodoroBreakMins(_breakMins);
                } else if (_selectedField == 2) {
                    _presetIndex = (_presetIndex + 1) % POMO_PRESET_COUNT;
                    applyPreset(_presetIndex);
                }
                return true;
            } else if (evt.button == BTN_ID_ENTER) {
                _state = POMO_IDLE;
                _totalSeconds = _focusMins * 60;
                _remainingSeconds = _totalSeconds;
                RenderEngine::getInstance().showToast("SAVED", "Focus Timer Ready", COLOR_NEON_MINT, 800);
                return true;
            }
            return false;
        }

        // --- IDLE MODE BUTTONS ---
        if (_state == POMO_IDLE) {
            if (evt.button == BTN_ID_ENTER || evt.button == BTN_ID_SELECT) {
                _state = POMO_FOCUS;
                _lastTick = millis();
                RenderEngine::getInstance().showToast("DEEP WORK", "Session Started", COLOR_NEON_CYAN, 800);
                return true;
            } else if (evt.button == BTN_ID_UP || evt.button == BTN_ID_DOWN) {
                _state = POMO_EDIT;
                _selectedField = 0;
                return true;
            } else if (evt.button == BTN_ID_LEFT) {
                _focusMins = (_focusMins > 5) ? (_focusMins - 5) : 1;
                setFocusMinutes(_focusMins);
                char toastBuf[24]; snprintf(toastBuf, sizeof(toastBuf), "%u Min Focus", _focusMins);
                RenderEngine::getInstance().showToast("TIMER SET", toastBuf, COLOR_NEON_CYAN, 600);
                return true;
            } else if (evt.button == BTN_ID_RIGHT) {
                _focusMins = (_focusMins <= 85) ? (_focusMins + 5) : 90;
                setFocusMinutes(_focusMins);
                char toastBuf[24]; snprintf(toastBuf, sizeof(toastBuf), "%u Min Focus", _focusMins);
                RenderEngine::getInstance().showToast("TIMER SET", toastBuf, COLOR_NEON_CYAN, 600);
                return true;
            }
            return false;
        }

        // --- RUNNING / PAUSED / BREAK BUTTONS ---
        if (evt.button == BTN_ID_ENTER || evt.button == BTN_ID_SELECT) {
            if (_state == POMO_FOCUS || _state == POMO_BREAK) {
                _state = POMO_PAUSED;
                RenderEngine::getInstance().showToast("PAUSED", "Timer Paused", COLOR_NEON_AMBER, 800);
            } else if (_state == POMO_PAUSED) {
                _state = POMO_FOCUS;
                _lastTick = millis();
                RenderEngine::getInstance().showToast("RESUMED", "Deep Work Resumed", COLOR_NEON_CYAN, 800);
            }
            return true;
        } else if (_state == POMO_PAUSED && (evt.button == BTN_ID_UP || evt.button == BTN_ID_DOWN)) {
            // Can enter edit from pause
            _state = POMO_EDIT;
            _selectedField = 0;
            _totalSeconds = _focusMins * 60;
            _remainingSeconds = _totalSeconds;
            return true;
        }
    }
    return false;
}
