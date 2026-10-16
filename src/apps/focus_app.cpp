#include "src/apps/focus_app.h"
#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/core/settings_manager.h"
#include "src/lighting/rgb_controller.h"
#include "src/lighting/indicator_led.h"
#include "src/display/theme.h"
#include <cmath>

REGISTER_APP(FocusApp);

void FocusApp::onInit() {
    uint16_t focusMins = SettingsManager::getInstance().getPomodoroFocusMins();
    _remainingSeconds = focusMins * 60;
    _totalSeconds = _remainingSeconds;
}

void FocusApp::onEnter() {}
void FocusApp::onExit() {}

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
                    _totalSeconds = 5 * 60; // 5 min break
                    _remainingSeconds = _totalSeconds;
                    RenderEngine::getInstance().showToast("FOCUS COMPLETE!", "Time for a 5m break", COLOR_NEON_MINT, 5000);
                    IndicatorLed::getInstance().flashOnce(500);
                    RgbController::getInstance().triggerFlash(0x2FE8FF, 1000);
                } else {
                    _state = POMO_IDLE;
                    uint16_t focusMins = SettingsManager::getInstance().getPomodoroFocusMins();
                    _totalSeconds = focusMins * 60;
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
                    RgbController::getInstance().triggerFlash(0xFF0000, 200);
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
    gfx.drawHeader("FOCUS FLOW", 4);

    int16_t cx = 64;
    int16_t cy = STATUS_BAR_HEIGHT + 48;
    int16_t radius = 34;

    // Progress Arc Calculation
    float progress = (_totalSeconds > 0) ? (1.0f - (float)_remainingSeconds / _totalSeconds) : 0.0f;

    // Dial ring
    gfx.drawCircle(cx, cy, radius, COLOR_SLATE_BORDER);
    gfx.drawCircle(cx, cy, radius - 1, COLOR_SLATE_MID);

    // Progress arc dots
    float maxAngle = progress * 6.283f - 1.57f;
    for (float a = -1.57f; a <= maxAngle; a += 0.2f) {
        int16_t px = cx + (int16_t)((radius - 2) * cos(a));
        int16_t py = cy + (int16_t)((radius - 2) * sin(a));
        uint16_t arcColor = (_state == POMO_BREAK) ? COLOR_NEON_MINT : COLOR_NEON_PURPLE;
        gfx.fillCircle(px, py, 2, arcColor);
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
    gfx.drawTextCentered(cx, cy + 14, stateText, stateColor, 1);

    // Noise Sentinel Status Banner
    if (_distractionAlert) {
        gfx.drawCard(8, STATUS_BAR_HEIGHT + 92, 112, 26, COLOR_SLATE_MID, COLOR_NEON_RED);
        gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 100, "! NOISY ROOM !", COLOR_NEON_RED, 1);
    } else {
        gfx.drawCard(8, STATUS_BAR_HEIGHT + 92, 112, 26, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
        TelemetryData telem = SharedData::getInstance().getData();
        char sentinelBuf[32];
        snprintf(sentinelBuf, sizeof(sentinelBuf), "Noise Sentinel: %.0f dB", telem.micDb);
        gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 100, sentinelBuf, COLOR_TEXT_MUTED, 1);
    }

    // Controls hint
    gfx.drawTextCentered(64, SCREEN_HEIGHT - 12, "ENTER: Play/Pause", COLOR_TEXT_MUTED, 1);
}

bool FocusApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK) {
        if (evt.button == BTN_ID_ENTER || evt.button == BTN_ID_SELECT) {
            if (_state == POMO_IDLE || _state == POMO_PAUSED) {
                _state = POMO_FOCUS;
                _lastTick = millis();
                RgbController::getInstance().setMode(RGB_BREATHE);
            } else if (_state == POMO_FOCUS || _state == POMO_BREAK) {
                _state = POMO_PAUSED;
            }
            return true;
        } else if (evt.button == BTN_ID_UP || evt.button == BTN_ID_DOWN || evt.button == BTN_ID_LEFT || evt.button == BTN_ID_RIGHT) {
            // Reset timer
            _state = POMO_IDLE;
            uint16_t focusMins = SettingsManager::getInstance().getPomodoroFocusMins();
            _totalSeconds = focusMins * 60;
            _remainingSeconds = _totalSeconds;
            return true;
        }
    }
    return false;
}
