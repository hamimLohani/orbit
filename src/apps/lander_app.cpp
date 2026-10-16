#include "src/apps/lander_app.h"
#include "src/core/app_registry.h"
#include "src/lighting/rgb_controller.h"
#include "src/display/theme.h"

REGISTER_APP(LanderApp);

void LanderApp::onInit() {
    resetGame();
}

void LanderApp::onEnter() {
    resetGame();
}

void LanderApp::onExit() {}

void LanderApp::resetGame() {
    _posX = 64.0f;
    _posY = 35.0f;
    _velX = (random(0, 2) == 0 ? 0.3f : -0.3f);
    _velY = 0.0f;
    _fuel = 100.0f;
    _thrusting = false;
    _gameOver = false;
    _landed = false;
}

void LanderApp::onUpdate(uint32_t deltaMs) {
    if (_gameOver || _landed) return;

    float dt = deltaMs / 1000.0f;
    const float gravity = 1.62f * 6.0f; // Moon gravity scaled

    // Physics
    _velY += gravity * dt;

    if (_thrusting && _fuel > 0.0f) {
        _velY -= 22.0f * dt;
        _fuel -= 18.0f * dt;
        if (_fuel < 0.0f) _fuel = 0.0f;
    }

    _posX += _velX * dt * 15.0f;
    _posY += _velY * dt * 15.0f;

    // Boundaries
    if (_posX < 10.0f) { _posX = 10.0f; _velX = -_velX * 0.5f; }
    if (_posX > SCREEN_WIDTH - 10) { _posX = SCREEN_WIDTH - 10; _velX = -_velX * 0.5f; }

    // Landing detection (Pad at Y = 135, X between 44 and 84)
    int16_t padY = SCREEN_HEIGHT - 22;
    int16_t padX1 = 44;
    int16_t padX2 = 84;

    if (_posY >= padY - 8) {
        _posY = padY - 8;
        // Check landing conditions
        if (_posX >= padX1 && _posX <= padX2 && fabs(_velY) < 1.8f && fabs(_velX) < 0.8f) {
            _landed = true;
            _score += 100 + (int)_fuel;
            RgbController::getInstance().triggerFlash(0x00FF00, 800);
        } else {
            _gameOver = true;
            RgbController::getInstance().triggerFlash(0xFF0000, 800);
        }
    }
}

void LanderApp::onRender(RenderEngine& gfx) {
    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);

    // Stars in background
    gfx.drawPixel(20, STATUS_BAR_HEIGHT + 10, COLOR_TEXT_MUTED);
    gfx.drawPixel(105, STATUS_BAR_HEIGHT + 18, COLOR_TEXT_MUTED);
    gfx.drawPixel(75, STATUS_BAR_HEIGHT + 45, COLOR_TEXT_MUTED);

    // HUD Stats at top
    char hudBuf[32];
    snprintf(hudBuf, sizeof(hudBuf), "F:%d%% V:%.1f", (int)_fuel, _velY);
    gfx.drawText(6, STATUS_BAR_HEIGHT + 2, hudBuf, COLOR_NEON_CYAN, 1);

    char scoreBuf[16];
    snprintf(scoreBuf, sizeof(scoreBuf), "S:%d", _score);
    gfx.drawTextRight(SCREEN_WIDTH - 6, STATUS_BAR_HEIGHT + 2, scoreBuf, COLOR_TEXT_LIGHT, 1);

    // Landing Pad (Center Bottom)
    int16_t padY = SCREEN_HEIGHT - 22;
    gfx.drawFastHLine(0, padY, 44, COLOR_SLATE_BORDER);
    gfx.drawFastHLine(44, padY, 40, COLOR_NEON_MINT); // Safe Zone
    gfx.drawFastHLine(84, padY, SCREEN_WIDTH - 84, COLOR_SLATE_BORDER);

    // Render Lander Module
    int16_t lx = (int16_t)_posX;
    int16_t ly = (int16_t)_posY;

    if (_gameOver) {
        // Crash explosion
        gfx.drawCircle(lx, ly, 8, COLOR_NEON_RED);
        gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 50, "CRASHED!", COLOR_NEON_RED, 2);
        gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 75, "SELECT to retry", COLOR_WHITE, 1);
    } else if (_landed) {
        // Successful landing
        gfx.drawCircle(lx, ly, 6, COLOR_NEON_MINT);
        gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 50, "LANDED!", COLOR_NEON_MINT, 2);
        gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 75, "SELECT for next", COLOR_WHITE, 1);
    } else {
        // Lander body (Triangle / Pod)
        gfx.drawCircle(lx, ly, 5, COLOR_TEXT_LIGHT);
        gfx.drawLine(lx - 5, ly + 5, lx - 7, ly + 8, COLOR_TEXT_MUTED); // Leg Left
        gfx.drawLine(lx + 5, ly + 5, lx + 7, ly + 8, COLOR_TEXT_MUTED); // Leg Right

        // Thruster flame if firing
        if (_thrusting && _fuel > 0.0f) {
            gfx.drawLine(lx - 2, ly + 6, lx, ly + 11, COLOR_NEON_AMBER);
            gfx.drawLine(lx + 2, ly + 6, lx, ly + 11, COLOR_NEON_AMBER);
        }
    }

    gfx.drawTextCentered(64, SCREEN_HEIGHT - 10, "UP: Thrust | DN: Steer", COLOR_TEXT_MUTED, 1);
}

bool LanderApp::onButtonEvent(const ButtonEvent& evt) {
    if (_gameOver || _landed) {
        if (evt.type == BUTTON_CLICK && evt.button == BTN_ID_SELECT) {
            resetGame();
            return true;
        }
    }

    if (evt.button == BTN_ID_UP) {
        if (evt.type == BUTTON_CLICK || evt.type == BUTTON_HOLD || evt.type == BUTTON_REPEAT) {
            _thrusting = true;
            return true;
        } else if (evt.type == BUTTON_RELEASE) {
            _thrusting = false;
            return true;
        }
    } else if (evt.button == BTN_ID_DOWN && evt.type == BUTTON_CLICK) {
        _velX = -_velX; // Invert drift
        return true;
    }
    return false;
}
