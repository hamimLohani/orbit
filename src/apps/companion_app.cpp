#include "src/apps/companion_app.h"
#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/lighting/rgb_controller.h"
#include "src/display/theme.h"
#include <cmath>

REGISTER_APP(CompanionApp);

void CompanionApp::onInit() {}
void CompanionApp::onEnter() {
    _petCounter = 0;
}
void CompanionApp::onExit() {}

void CompanionApp::onUpdate(uint32_t deltaMs) {
    uint32_t now = millis();
    TelemetryData telem = SharedData::getInstance().getData();

    // 1. Natural eye blinking
    if (!_isBlinking && (now - _blinkTimer > _nextBlinkInterval)) {
        _isBlinking = true;
        _blinkTimer = now;
    } else if (_isBlinking && (now - _blinkTimer > 160)) {
        _isBlinking = false;
        _blinkTimer = now;
        _nextBlinkInterval = random(2000, 6000);
    }

    // 2. Idle eye gaze roaming
    if (now - _lookTimer > 3500) {
        _lookTimer = now;
        _eyeOffsetX = random(-4, 5);
        _eyeOffsetY = random(-3, 4);
    }

    // 3. Ambient Mood Engine
    if (telem.micDb > 75.0f) {
        _mood = MOOD_SURPRISED;
    } else if (telem.micDb > 62.0f) {
        _mood = MOOD_DISCO;
    } else if (telem.temperature < 18.0f) {
        _mood = MOOD_COLD;
    } else if (telem.temperature > 28.0f) {
        _mood = MOOD_HOT;
    } else if (telem.isNightTime) {
        _mood = MOOD_SLEEPY;
        _sleepBubbleY += 0.8f * (deltaMs / 1000.0f);
        if (_sleepBubbleY > 1.0f) _sleepBubbleY = 0.0f;
    } else {
        _mood = MOOD_HAPPY;
    }
}

void CompanionApp::onRender(RenderEngine& gfx) {
    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);
    gfx.drawHeader("AURA COMPANION", 1);

    int16_t cx = 64;
    int16_t cy = STATUS_BAR_HEIGHT + 44; // y = 58

    // Outer Aura Halo (radius 20, spans y=38..78, cleanly below header line at y=37)
    uint16_t haloColor = COLOR_NEON_CYAN;
    if (_mood == MOOD_COLD) haloColor = COLOR_NEON_BLUE;
    else if (_mood == MOOD_HOT) haloColor = COLOR_NEON_CORAL;
    else if (_mood == MOOD_SLEEPY) haloColor = COLOR_NEON_PURPLE;
    else if (_mood == MOOD_DISCO) haloColor = COLOR_NEON_AMBER;

    gfx.drawCircle(cx, cy, 21, COLOR_SLATE_BORDER);
    gfx.drawCircle(cx, cy, 18, haloColor);

    // Render Procedural Eyes
    renderEyes(gfx, cx, cy);

    // Status / Reaction Text
    const char* status = "Feeling peaceful";
    if (_mood == MOOD_COLD) status = "*Shivering* Cold!";
    else if (_mood == MOOD_HOT) status = "*Puffing* Too warm!";
    else if (_mood == MOOD_SURPRISED) status = "Woah! Loud sound!";
    else if (_mood == MOOD_DISCO) status = "Grooving to music!";
    else if (_mood == MOOD_SLEEPY) status = "Zzz... Snoozing";
    else if (_petCounter > 0) status = "Happy friend! <3";

    gfx.drawCard(8, STATUS_BAR_HEIGHT + 74, SCREEN_WIDTH - 16, 26, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 83, status, COLOR_TEXT_LIGHT, 1);

    // Prompt hint
    gfx.drawTextCentered(64, SCREEN_HEIGHT - 10, "ENTER: Pet Aura", COLOR_TEXT_MUTED, 1);
}

void CompanionApp::renderEyes(RenderEngine& gfx, int16_t cx, int16_t cy) {
    int16_t eyeSpacing = 10;
    int16_t leftEyeX = cx - eyeSpacing;
    int16_t rightEyeX = cx + eyeSpacing;
    int16_t eyeY = cy - 3;

    if (_isBlinking || _mood == MOOD_SLEEPY) {
        // Closed / Sleeping eyes: Curved lines
        gfx.drawFastHLine(leftEyeX - 4, eyeY, 9, COLOR_NEON_CYAN);
        gfx.drawFastHLine(rightEyeX - 4, eyeY, 9, COLOR_NEON_CYAN);

        // Sleep Zzz bubbles
        if (_mood == MOOD_SLEEPY) {
            int16_t zY = cy - 12 - (int16_t)(_sleepBubbleY * 12);
            gfx.drawText(cx + 14, zY, "z", COLOR_TEXT_MUTED, 1);
            gfx.drawText(cx + 19, zY - 5, "Z", COLOR_TEXT_LIGHT, 1);
        }
        return;
    }

    if (_mood == MOOD_HAPPY) {
        // Cheerful arc eyes
        gfx.drawCircle(leftEyeX, eyeY, 4, COLOR_NEON_CYAN);
        gfx.fillCircle(leftEyeX + constrain(_eyeOffsetX, -2, 2), eyeY + constrain(_eyeOffsetY, -2, 2), 2, COLOR_WHITE);

        gfx.drawCircle(rightEyeX, eyeY, 4, COLOR_NEON_CYAN);
        gfx.fillCircle(rightEyeX + constrain(_eyeOffsetX, -2, 2), eyeY + constrain(_eyeOffsetY, -2, 2), 2, COLOR_WHITE);

        // Smile
        gfx.drawFastHLine(cx - 4, cy + 9, 8, COLOR_NEON_CYAN);
    } else if (_mood == MOOD_SURPRISED) {
        // Big round startled eyes
        gfx.fillCircle(leftEyeX, eyeY, 6, COLOR_WHITE);
        gfx.fillCircle(leftEyeX, eyeY, 3, COLOR_VOID_DARK);

        gfx.fillCircle(rightEyeX, eyeY, 6, COLOR_WHITE);
        gfx.fillCircle(rightEyeX, eyeY, 3, COLOR_VOID_DARK);

        // Open mouth 'O'
        gfx.drawCircle(cx, cy + 10, 3, COLOR_NEON_AMBER);
    } else if (_mood == MOOD_COLD) {
        // Shivering eyes
        int16_t jitter = (millis() / 50) % 2 ? 1 : -1;
        gfx.drawCircle(leftEyeX + jitter, eyeY, 4, COLOR_NEON_BLUE);
        gfx.drawCircle(rightEyeX + jitter, eyeY, 4, COLOR_NEON_BLUE);
        gfx.drawText(cx - 8, cy + 8, "~~~", COLOR_NEON_BLUE, 1);
    } else if (_mood == MOOD_HOT) {
        // Drooping eyes
        gfx.drawCircle(leftEyeX, eyeY, 4, COLOR_NEON_CORAL);
        gfx.drawCircle(rightEyeX, eyeY, 4, COLOR_NEON_CORAL);
        // Sweat droplet
        gfx.fillCircle(cx + 14, cy - 8, 2, COLOR_NEON_CYAN);
    } else if (_mood == MOOD_DISCO) {
        // Musical sunglasses or dancing eyes
        gfx.fillRect(leftEyeX - 5, eyeY - 3, 10, 6, COLOR_NEON_AMBER);
        gfx.fillRect(rightEyeX - 5, eyeY - 3, 10, 6, COLOR_NEON_AMBER);
        gfx.drawFastHLine(leftEyeX, eyeY, rightEyeX - leftEyeX, COLOR_NEON_AMBER);
    }
}

bool CompanionApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK && (evt.button == BTN_ID_ENTER || evt.button == BTN_ID_SELECT)) {
        _petCounter++;
        _mood = MOOD_HAPPY;
        RgbController::getInstance().triggerFlash(0xFF69B4, 300); // Pink love flash
        return true;
    }
    return false;
}

void CompanionApp::onEvent(EventType type, const void* payload) {
    if (type == EVENT_SOUND_SPIKE) {
        _mood = MOOD_SURPRISED;
    }
}
