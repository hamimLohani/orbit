#include "src/apps/snake_app.h"
#include "src/core/app_registry.h"
#include "src/core/settings_manager.h"
#include "src/lighting/rgb_controller.h"
#include "src/display/theme.h"

REGISTER_APP(SnakeApp);

void SnakeApp::onInit() {
    _highScore = getHighScore(_selectedMode);
    resetToMenu();
}

void SnakeApp::onEnter() {
    _highScore = getHighScore(_selectedMode);
    resetToMenu();
}

void SnakeApp::onExit() {
    // If exiting during game, persist high score
    if (_score > _highScore) {
        saveHighScore(_selectedMode, _score);
        _highScore = _score;
    }
}

int SnakeApp::getHighScore(SnakeMode mode) {
    char key[16];
    snprintf(key, sizeof(key), "hi_m%d", (int)mode);
    return SettingsManager::getInstance().getAppInt("snake", key, 0);
}

void SnakeApp::saveHighScore(SnakeMode mode, int score) {
    char key[16];
    snprintf(key, sizeof(key), "hi_m%d", (int)mode);
    SettingsManager::getInstance().setAppInt("snake", key, score);
}

void SnakeApp::resetToMenu() {
    _state = SNAKE_STATE_MENU;
    _menuRow = 0;
    _highScore = getHighScore(_selectedMode);
}

void SnakeApp::startNewGame() {
    _state = SNAKE_STATE_PLAYING;
    _score = 0;
    _newHighScore = false;
    _deathReason = "";
    _length = 3;
    _growing = false;

    // Reset Direction & Queue
    _dir = DIR_RIGHT;
    _queueLen = 0;

    // Start snake at center
    int8_t startX = GRID_W / 2;
    int8_t startY = GRID_H / 2;
    _body[0] = { startX, startY };
    _body[1] = { (int8_t)(startX - 1), startY };
    _body[2] = { (int8_t)(startX - 2), startY };

    // Reset Timers
    switch (_difficulty) {
        case DIFF_EASY:   _baseTickMs = 150; break;
        case DIFF_NORMAL: _baseTickMs = 110; break;
        case DIFF_HARD:   _baseTickMs = 75;  break;
        default:          _baseTickMs = 110; break;
    }
    _currentTickMs = _baseTickMs;
    _tickTimer = 0;

    _timeAttackRemaining = 60.0f;
    _ghostTimer = 0.0f;
    _slowTimer = 0.0f;
    _multiplierTimer = 0.0f;
    _poisonTimer = 0.0f;

    _specialType = ITEM_NONE;
    _specialPos = { -1, -1 };
    _specialSpawnTimer = 10000; // 10s till first power-up

    // Obstacles
    _numObstacles = 0;
    if (_obstaclesEnabled || _selectedMode == SNAKE_MODE_SURVIVAL) {
        generateObstacles();
    }

    _highScore = getHighScore(_selectedMode);

    spawnFood();
}

void SnakeApp::generateObstacles() {
    _numObstacles = 0;

    // Symmetrical Pillars Pattern (Corners)
    const int8_t pillars[][2] = {
        { 3, 3 }, { 4, 3 }, { 3, 4 }, { 4, 4 },
        { (int8_t)(GRID_W - 5), 3 }, { (int8_t)(GRID_W - 4), 3 }, { (int8_t)(GRID_W - 5), 4 }, { (int8_t)(GRID_W - 4), 4 },
        { 3, (int8_t)(GRID_H - 5) }, { 4, (int8_t)(GRID_H - 5) }, { 3, (int8_t)(GRID_H - 4) }, { 4, (int8_t)(GRID_H - 4) },
        { (int8_t)(GRID_W - 5), (int8_t)(GRID_H - 5) }, { (int8_t)(GRID_W - 4), (int8_t)(GRID_H - 5) },
        { (int8_t)(GRID_W - 5), (int8_t)(GRID_H - 4) }, { (int8_t)(GRID_W - 4), (int8_t)(GRID_H - 4) }
    };

    uint8_t count = sizeof(pillars) / sizeof(pillars[0]);
    for (uint8_t i = 0; i < count && _numObstacles < MAX_OBSTACLES; ++i) {
        _obstacles[_numObstacles++] = { pillars[i][0], pillars[i][1] };
    }

    // In Survival mode, add center horizontal split barriers
    if (_selectedMode == SNAKE_MODE_SURVIVAL) {
        for (int8_t x = 7; x <= 12; ++x) {
            if (_numObstacles < MAX_OBSTACLES) _obstacles[_numObstacles++] = { x, 7 };
            if (_numObstacles < MAX_OBSTACLES) _obstacles[_numObstacles++] = { x, 15 };
        }
    }
}

bool SnakeApp::isCellOccupied(int8_t x, int8_t y, bool includeSnake, bool includeObstacles) const {
    if (includeSnake) {
        for (uint16_t i = 0; i < _length; ++i) {
            if (_body[i].x == x && _body[i].y == y) return true;
        }
    }
    if (includeObstacles) {
        for (uint8_t i = 0; i < _numObstacles; ++i) {
            if (_obstacles[i].x == x && _obstacles[i].y == y) return true;
        }
    }
    return false;
}

void SnakeApp::spawnFood() {
    uint16_t attempts = 0;
    while (attempts < 200) {
        int8_t rx = random(0, GRID_W);
        int8_t ry = random(0, GRID_H);

        if (!isCellOccupied(rx, ry, true, true)) {
            if (_specialType == ITEM_NONE || !(_specialPos.x == rx && _specialPos.y == ry)) {
                _foodPos = { rx, ry };
                return;
            }
        }
        attempts++;
    }
    _foodPos = { 2, 2 };
}

void SnakeApp::spawnSpecialItem() {
    uint16_t attempts = 0;
    while (attempts < 200) {
        int8_t rx = random(0, GRID_W);
        int8_t ry = random(0, GRID_H);

        if (!isCellOccupied(rx, ry, true, true) && !(rx == _foodPos.x && ry == _foodPos.y)) {
            _specialPos = { rx, ry };
            // Randomly select power-up type from the specification
            uint8_t roll = random(0, 5);
            switch (roll) {
                case 0: _specialType = ITEM_SLOW; break;
                case 1: _specialType = ITEM_GHOST; break;
                case 2: _specialType = ITEM_3X; break;
                case 3: _specialType = ITEM_SHRINK; break;
                case 4: _specialType = ITEM_POISON; break;
                default: _specialType = ITEM_GHOST; break;
            }
            _specialDespawnTimer = 10000; // Remains for 10s
            return;
        }
        attempts++;
    }
}

void SnakeApp::queueDirection(SnakeDirection newDir) {
    if (_queueLen >= 2) return;

    SnakeDirection compareDir = (_queueLen > 0) ? _dirQueue[_queueLen - 1] : _dir;

    // Anti-180° instant suicide check
    if ((newDir == DIR_UP && compareDir == DIR_DOWN) ||
        (newDir == DIR_DOWN && compareDir == DIR_UP) ||
        (newDir == DIR_LEFT && compareDir == DIR_RIGHT) ||
        (newDir == DIR_RIGHT && compareDir == DIR_LEFT)) {
        return;
    }

    _dirQueue[_queueLen++] = newDir;
}

void SnakeApp::triggerGameOver(const char* reason) {
    _state = SNAKE_STATE_GAME_OVER;
    _deathReason = reason;

    if (_score > _highScore) {
        _highScore = _score;
        _newHighScore = true;
        saveHighScore(_selectedMode, _score);
        RgbController::getInstance().triggerFlash(0xFFD700, 1000); // Gold celebration
    } else {
        RgbController::getInstance().triggerFlash(0xFF0000, 800);  // Red crash
    }
}

void SnakeApp::onUpdate(uint32_t deltaMs) {
    if (_state != SNAKE_STATE_PLAYING) return;

    float dt = deltaMs / 1000.0f;

    // 1. Update Active Power-Up Timers
    if (_ghostTimer > 0.0f) _ghostTimer -= dt;
    if (_slowTimer > 0.0f) _slowTimer -= dt;
    if (_multiplierTimer > 0.0f) _multiplierTimer -= dt;
    if (_poisonTimer > 0.0f) _poisonTimer -= dt;

    // 2. Adaptive Speed & Modifier Calculations
    if (_slowTimer > 0.0f) {
        _currentTickMs = _baseTickMs * 2; // Half speed
    } else if (_poisonTimer > 0.0f) {
        _currentTickMs = max(40, (int)_baseTickMs / 2); // Toxic speed surge
    } else {
        // Subtle progressive speed up with score
        int speedBonus = (_score / 50) * 4;
        _currentTickMs = (speedBonus < (int)_baseTickMs - 45) ? (_baseTickMs - speedBonus) : 45;
    }

    // 3. Time Attack Countdown Clock
    if (_selectedMode == SNAKE_MODE_TIME_ATTACK) {
        _timeAttackRemaining -= dt;
        if (_timeAttackRemaining <= 0.0f) {
            _timeAttackRemaining = 0.0f;
            triggerGameOver("Time Expired!");
            return;
        }
    }

    // 4. Special Item Spawning & Despawning
    if (_specialType == ITEM_NONE) {
        if (_specialSpawnTimer > deltaMs) {
            _specialSpawnTimer -= deltaMs;
        } else {
            spawnSpecialItem();
        }
    } else {
        if (_specialDespawnTimer > deltaMs) {
            _specialDespawnTimer -= deltaMs;
        } else {
            _specialType = ITEM_NONE;
            _specialPos = { -1, -1 };
            _specialSpawnTimer = random(12000, 18000); // 12-18s until next power-up
        }
    }

    // 5. Game Tick Execution
    _tickTimer += deltaMs;
    if (_tickTimer >= _currentTickMs) {
        _tickTimer = 0;
        tickGame();
    }
}

void SnakeApp::tickGame() {
    // 1. Pop Direction Queue
    if (_queueLen > 0) {
        _dir = _dirQueue[0];
        _dirQueue[0] = _dirQueue[1];
        _queueLen--;
    }

    // 2. Compute New Head Position
    int8_t nextX = _body[0].x;
    int8_t nextY = _body[0].y;

    if (_dir == DIR_UP) nextY--;
    else if (_dir == DIR_DOWN) nextY++;
    else if (_dir == DIR_LEFT) nextX--;
    else if (_dir == DIR_RIGHT) nextX++;

    // 3. Wall Collision / Toroidal Wrap-around
    bool ghostActive = (_ghostTimer > 0.0f);
    bool zenMode = (_selectedMode == SNAKE_MODE_ZEN);

    if (ghostActive || zenMode) {
        // Toroidal screen wrap
        if (nextX < 0) nextX = GRID_W - 1;
        else if (nextX >= GRID_W) nextX = 0;
        if (nextY < 0) nextY = GRID_H - 1;
        else if (nextY >= GRID_H) nextY = 0;
    } else {
        // Solid Wall Collision
        if (nextX < 0 || nextX >= GRID_W || nextY < 0 || nextY >= GRID_H) {
            triggerGameOver("Wall Crash!");
            return;
        }
    }

    // 4. Obstacle Collision
    if (!ghostActive) {
        for (uint8_t i = 0; i < _numObstacles; ++i) {
            if (_obstacles[i].x == nextX && _obstacles[i].y == nextY) {
                triggerGameOver("Obstacle Hit!");
                return;
            }
        }
    }

    // 5. Self Collision (Tail Vacating Rule)
    if (!ghostActive && !zenMode) {
        uint16_t checkLen = _growing ? _length : (_length - 1);
        for (uint16_t i = 1; i < checkLen; ++i) {
            if (_body[i].x == nextX && _body[i].y == nextY) {
                triggerGameOver("Tail Bite!");
                return;
            }
        }
    }

    // 6. Shift Body Segments
    if (_growing) {
        if (_length < MAX_SNAKE_LEN) _length++;
        _growing = false;
    }

    for (int16_t i = _length - 1; i > 0; --i) {
        _body[i] = _body[i - 1];
    }
    _body[0] = { nextX, nextY };

    // 7. Check Food Pickup
    if (nextX == _foodPos.x && nextY == _foodPos.y) {
        _growing = true;
        int mult = (_multiplierTimer > 0.0f) ? 3 : 1;
        int pts = (_selectedMode == SNAKE_MODE_TIME_ATTACK ? 25 : 10) * mult;
        _score += pts;

        if (_selectedMode == SNAKE_MODE_TIME_ATTACK) {
            _timeAttackRemaining += 3.0f; // +3s bonus per apple
        }

        RgbController::getInstance().triggerFlash(0x00FF00, 200);
        spawnFood();
    }

    // 8. Check Special Item Pickup
    if (_specialType != ITEM_NONE && nextX == _specialPos.x && nextY == _specialPos.y) {
        handleItemPickup(_specialType);
        _specialType = ITEM_NONE;
        _specialPos = { -1, -1 };
        _specialSpawnTimer = random(12000, 18000);
    }
}

void SnakeApp::handleItemPickup(SnakeItemType item) {
    int mult = (_multiplierTimer > 0.0f) ? 3 : 1;

    switch (item) {
        case ITEM_SLOW:
            _slowTimer = 5.0f;
            _score += 20 * mult;
            RgbController::getInstance().triggerFlash(0x00FFFF, 400);
            break;

        case ITEM_GHOST:
            _ghostTimer = 10.0f; // 10s Zen mode!
            _score += 20 * mult;
            RgbController::getInstance().triggerFlash(0xD500F9, 500);
            break;

        case ITEM_3X:
            _multiplierTimer = 5.0f;
            _score += 30 * mult;
            RgbController::getInstance().triggerFlash(0xFFD700, 400);
            break;

        case ITEM_SHRINK:
            _length = max(3, (int)_length / 2);
            _score += 15 * mult;
            RgbController::getInstance().triggerFlash(0xFF6D00, 300);
            break;

        case ITEM_POISON:
            _score = max(0, _score - 30);
            _growing = true;     // +1 unrewarded length
            _poisonTimer = 5.0f; // 5s toxic speed surge
            RgbController::getInstance().triggerFlash(0xFF0055, 600);
            break;

        default:
            break;
    }
}

// ------------------------------------------------------------------------------
// Rendering Pipeline
// ------------------------------------------------------------------------------
void SnakeApp::onRender(RenderEngine& gfx) {
    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);

    switch (_state) {
        case SNAKE_STATE_MENU:
            renderMenu(gfx);
            break;
        case SNAKE_STATE_PLAYING:
            renderHUD(gfx);
            renderArena(gfx);
            renderObstacles(gfx);
            renderItems(gfx);
            renderSnake(gfx);
            break;
        case SNAKE_STATE_PAUSED:
            renderHUD(gfx);
            renderArena(gfx);
            renderObstacles(gfx);
            renderItems(gfx);
            renderSnake(gfx);
            renderPaused(gfx);
            break;
        case SNAKE_STATE_GAME_OVER:
            renderHUD(gfx);
            renderArena(gfx);
            renderObstacles(gfx);
            renderItems(gfx);
            renderSnake(gfx);
            renderGameOver(gfx);
            break;
    }
}

void SnakeApp::renderHUD(RenderEngine& gfx) {
    // Mode Tag & Clock / Score Header
    char hudBuf[24];
    if (_selectedMode == SNAKE_MODE_TIME_ATTACK) {
        snprintf(hudBuf, sizeof(hudBuf), "TIME:%ds", (int)ceil(_timeAttackRemaining));
    } else if (_selectedMode == SNAKE_MODE_ZEN) {
        snprintf(hudBuf, sizeof(hudBuf), "ZEN");
    } else if (_selectedMode == SNAKE_MODE_SURVIVAL) {
        snprintf(hudBuf, sizeof(hudBuf), "SURV");
    } else {
        snprintf(hudBuf, sizeof(hudBuf), "CLS");
    }
    gfx.drawText(6, STATUS_BAR_HEIGHT + 2, hudBuf, COLOR_TEXT_MUTED, 1);

    // Active Power-Up Notification pill in center
    if (_ghostTimer > 0.0f) {
        char pBuf[16];
        snprintf(pBuf, sizeof(pBuf), "GHOST:%ds", (int)ceil(_ghostTimer));
        gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 2, pBuf, COLOR_NEON_PURPLE, 1);
    } else if (_multiplierTimer > 0.0f) {
        char pBuf[16];
        snprintf(pBuf, sizeof(pBuf), "3X:%ds", (int)ceil(_multiplierTimer));
        gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 2, pBuf, COLOR_NEON_AMBER, 1);
    } else if (_slowTimer > 0.0f) {
        char pBuf[16];
        snprintf(pBuf, sizeof(pBuf), "SLOW:%ds", (int)ceil(_slowTimer));
        gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 2, pBuf, COLOR_NEON_CYAN, 1);
    } else if (_poisonTimer > 0.0f) {
        char pBuf[16];
        snprintf(pBuf, sizeof(pBuf), "SURGE:%ds", (int)ceil(_poisonTimer));
        gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 2, pBuf, COLOR_NEON_RED, 1);
    }

    // Score on right
    char scoreBuf[16];
    snprintf(scoreBuf, sizeof(scoreBuf), "S:%d", _score);
    gfx.drawTextRight(SCREEN_WIDTH - 6, STATUS_BAR_HEIGHT + 2, scoreBuf, COLOR_NEON_CYAN, 1);
}

void SnakeApp::renderArena(RenderEngine& gfx) {
    // Arena Border (Pulsing purple in Ghost Mode!)
    uint16_t borderColor = COLOR_SLATE_BORDER;
    if (_ghostTimer > 0.0f) {
        borderColor = (millis() % 400 < 200) ? COLOR_NEON_PURPLE : COLOR_VOID_DARK;
    } else if (_selectedMode == SNAKE_MODE_ZEN) {
        borderColor = COLOR_SLATE_MID;
    }

    gfx.drawRect(ARENA_X - 1, ARENA_Y - 1, GRID_W * CELL_SIZE + 2, GRID_H * CELL_SIZE + 2, borderColor);
}

void SnakeApp::renderObstacles(RenderEngine& gfx) {
    for (uint8_t i = 0; i < _numObstacles; ++i) {
        int16_t px = ARENA_X + _obstacles[i].x * CELL_SIZE;
        int16_t py = ARENA_Y + _obstacles[i].y * CELL_SIZE;
        gfx.fillRect(px, py, CELL_SIZE, CELL_SIZE, COLOR_SLATE_MID);
        gfx.drawRect(px, py, CELL_SIZE, CELL_SIZE, COLOR_SLATE_BORDER);
    }
}

void SnakeApp::renderItems(RenderEngine& gfx) {
    // 1. Food Apple (Glowing Red with Green Stem)
    int16_t fx = ARENA_X + _foodPos.x * CELL_SIZE;
    int16_t fy = ARENA_Y + _foodPos.y * CELL_SIZE;
    gfx.fillRect(fx + 1, fy + 1, CELL_SIZE - 2, CELL_SIZE - 2, COLOR_NEON_RED);
    gfx.drawPixel(fx + 2, fy, COLOR_NEON_MINT); // Green leaf

    // 2. Special Power-Up Item
    if (_specialType != ITEM_NONE && _specialPos.x >= 0) {
        int16_t sx = ARENA_X + _specialPos.x * CELL_SIZE;
        int16_t sy = ARENA_Y + _specialPos.y * CELL_SIZE;

        // Flashing effect in last 3 seconds
        if (_specialDespawnTimer < 3000 && (millis() % 200 < 100)) {
            return;
        }

        uint16_t itemColor = COLOR_NEON_CYAN;
        switch (_specialType) {
            case ITEM_SLOW:   itemColor = COLOR_NEON_CYAN;   break;
            case ITEM_GHOST:  itemColor = COLOR_NEON_PURPLE; break;
            case ITEM_3X:     itemColor = COLOR_NEON_AMBER;  break;
            case ITEM_SHRINK: itemColor = 0xFD20;            break; // Orange
            case ITEM_POISON: itemColor = COLOR_NEON_RED;    break;
            default: break;
        }

        gfx.fillCircle(sx + 3, sy + 3, 2, itemColor);
        gfx.drawPixel(sx + 3, sy + 3, COLOR_TEXT_LIGHT);
    }
}

void SnakeApp::renderSnake(RenderEngine& gfx) {
    bool ghost = (_ghostTimer > 0.0f);

    // 1. Render Body
    for (uint16_t i = 1; i < _length; ++i) {
        int16_t bx = ARENA_X + _body[i].x * CELL_SIZE;
        int16_t by = ARENA_Y + _body[i].y * CELL_SIZE;

        uint16_t bodyColor = ghost ? COLOR_NEON_PURPLE : COLOR_NEON_CYAN;
        if (ghost && (i % 2 == 0)) bodyColor = 0x780F; // Translucent purple pattern

        gfx.fillRect(bx + 1, by + 1, CELL_SIZE - 2, CELL_SIZE - 2, bodyColor);
    }

    // 2. Render Head (Neon Mint with directional eyes)
    int16_t hx = ARENA_X + _body[0].x * CELL_SIZE;
    int16_t hy = ARENA_Y + _body[0].y * CELL_SIZE;
    uint16_t headColor = ghost ? COLOR_NEON_PURPLE : COLOR_NEON_MINT;

    gfx.fillRect(hx, hy, CELL_SIZE, CELL_SIZE, headColor);

    // Draw eye dots based on direction
    if (_dir == DIR_RIGHT) {
        gfx.drawPixel(hx + 4, hy + 1, COLOR_VOID_DARK);
        gfx.drawPixel(hx + 4, hy + 4, COLOR_VOID_DARK);
    } else if (_dir == DIR_LEFT) {
        gfx.drawPixel(hx + 1, hy + 1, COLOR_VOID_DARK);
        gfx.drawPixel(hx + 1, hy + 4, COLOR_VOID_DARK);
    } else if (_dir == DIR_UP) {
        gfx.drawPixel(hx + 1, hy + 1, COLOR_VOID_DARK);
        gfx.drawPixel(hx + 4, hy + 1, COLOR_VOID_DARK);
    } else if (_dir == DIR_DOWN) {
        gfx.drawPixel(hx + 1, hy + 4, COLOR_VOID_DARK);
        gfx.drawPixel(hx + 4, hy + 4, COLOR_VOID_DARK);
    }
}

void SnakeApp::renderMenu(RenderEngine& gfx) {
    gfx.drawHeader("SNAKE RETRO", 12);

    // Main Settings Card - starts at STATUS_BAR_HEIGHT + 26 (y=40), cleanly below header line (y=37)
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 26, 112, 74, COLOR_SLATE_MID, COLOR_SLATE_BORDER);

    // Row 0: Mode
    const char* modeNames[] = { "Classic", "TimeAtk", "Survive", "Zen" };
    uint16_t c0 = (_menuRow == 0) ? COLOR_NEON_CYAN : COLOR_TEXT_LIGHT;
    gfx.drawText(14, STATUS_BAR_HEIGHT + 31, (_menuRow == 0 ? "> Mode:" : "  Mode:"), c0, 1);
    gfx.drawTextRight(112, STATUS_BAR_HEIGHT + 31, modeNames[_selectedMode], (_menuRow == 0 ? COLOR_NEON_MINT : COLOR_TEXT_MUTED), 1);

    // Row 1: Obstacles
    uint16_t c1 = (_menuRow == 1) ? COLOR_NEON_CYAN : COLOR_TEXT_LIGHT;
    gfx.drawText(14, STATUS_BAR_HEIGHT + 44, (_menuRow == 1 ? "> Mazes:" : "  Mazes:"), c1, 1);
    gfx.drawTextRight(112, STATUS_BAR_HEIGHT + 44, (_obstaclesEnabled ? "ON" : "OFF"), (_obstaclesEnabled ? COLOR_NEON_AMBER : COLOR_TEXT_MUTED), 1);

    // Row 2: Difficulty
    const char* diffNames[] = { "Easy", "Normal", "Hard" };
    uint16_t c2 = (_menuRow == 2) ? COLOR_NEON_CYAN : COLOR_TEXT_LIGHT;
    gfx.drawText(14, STATUS_BAR_HEIGHT + 57, (_menuRow == 2 ? "> Speed:" : "  Speed:"), c2, 1);
    gfx.drawTextRight(112, STATUS_BAR_HEIGHT + 57, diffNames[_difficulty], (_menuRow == 2 ? COLOR_NEON_CYAN : COLOR_TEXT_MUTED), 1);

    // Row 3: Start Button Action
    uint16_t c3 = (_menuRow == 3) ? COLOR_NEON_MINT : COLOR_TEXT_MUTED;
    gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 73, (_menuRow == 3 ? "[ START GAME ]" : "START GAME"), c3, 1);

    // High Score Box - cleanly separated at y = 118, height 22 -> ends at 140
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 104, 112, 22, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    char hiBuf[24];
    snprintf(hiBuf, sizeof(hiBuf), "RECORD: %d PTS", _highScore);
    gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 111, hiBuf, COLOR_NEON_AMBER, 1);

    // Bottom Navigation Hint - 21 characters (126px), fits screen width 128px at y=150
    gfx.drawTextCentered(64, SCREEN_HEIGHT - 10, "U/D:Nav L/R:Set OK:Go", COLOR_TEXT_MUTED, 1);
}

void SnakeApp::renderPaused(RenderEngine& gfx) {
    // Semi-opaque overlay box
    gfx.fillRoundRect(16, 60, 96, 44, 4, COLOR_SLATE_DARK);
    gfx.drawRoundRect(16, 60, 96, 44, 4, COLOR_NEON_CYAN);

    gfx.drawTextCentered(64, 68, "PAUSED", COLOR_NEON_CYAN, 2);
    gfx.drawTextCentered(64, 88, "[OK] Resume  [<] Menu", COLOR_TEXT_LIGHT, 1);
}

void SnakeApp::renderGameOver(RenderEngine& gfx) {
    gfx.fillRoundRect(12, 45, 104, 76, 6, COLOR_SLATE_DARK);
    gfx.drawRoundRect(12, 45, 104, 76, 6, _newHighScore ? COLOR_NEON_AMBER : COLOR_NEON_RED);

    gfx.drawTextCentered(64, 52, "GAME OVER", COLOR_NEON_RED, 2);
    gfx.drawTextCentered(64, 70, _deathReason, COLOR_TEXT_MUTED, 1);

    char sBuf[24];
    snprintf(sBuf, sizeof(sBuf), "Score: %d", _score);
    gfx.drawTextCentered(64, 82, sBuf, COLOR_TEXT_LIGHT, 1);

    if (_newHighScore) {
        gfx.drawTextCentered(64, 93, "* NEW HIGH SCORE *", COLOR_NEON_AMBER, 1);
    } else {
        char hiBuf[20];
        snprintf(hiBuf, sizeof(hiBuf), "Record: %d", _highScore);
        gfx.drawTextCentered(64, 93, hiBuf, COLOR_TEXT_MUTED, 1);
    }

    gfx.drawTextCentered(64, 107, "[OK] Retry  [<] Menu", COLOR_NEON_CYAN, 1);
}

// ------------------------------------------------------------------------------
// Input Handling
// ------------------------------------------------------------------------------
bool SnakeApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type != BUTTON_CLICK && evt.type != BUTTON_REPEAT) return false;

    // 1. In Menu State
    if (_state == SNAKE_STATE_MENU) {
        if (evt.button == BTN_ID_UP) {
            _menuRow = (_menuRow > 0) ? (_menuRow - 1) : 3;
            return true;
        } else if (evt.button == BTN_ID_DOWN) {
            _menuRow = (_menuRow < 3) ? (_menuRow + 1) : 0;
            return true;
        } else if (evt.button == BTN_ID_LEFT || evt.button == BTN_ID_RIGHT) {
            int dir = (evt.button == BTN_ID_RIGHT) ? 1 : -1;
            if (_menuRow == 0) {
                int nextMode = (int)_selectedMode + dir;
                if (nextMode < 0) nextMode = SNAKE_MODE_COUNT - 1;
                if (nextMode >= SNAKE_MODE_COUNT) nextMode = 0;
                _selectedMode = (SnakeMode)nextMode;
                _highScore = getHighScore(_selectedMode);
            } else if (_menuRow == 1) {
                _obstaclesEnabled = !_obstaclesEnabled;
            } else if (_menuRow == 2) {
                int nextDiff = (int)_difficulty + dir;
                if (nextDiff < 0) nextDiff = DIFF_COUNT - 1;
                if (nextDiff >= DIFF_COUNT) nextDiff = 0;
                _difficulty = (SnakeDifficulty)nextDiff;
            } else if (_menuRow == 3) {
                startNewGame();
            }
            return true;
        } else if (evt.button == BTN_ID_ENTER) {
            startNewGame();
            return true;
        }
        return false;
    }

    // 2. In Playing State
    if (_state == SNAKE_STATE_PLAYING) {
        if (evt.button == BTN_ID_UP) {
            queueDirection(DIR_UP);
            return true;
        } else if (evt.button == BTN_ID_DOWN) {
            queueDirection(DIR_DOWN);
            return true;
        } else if (evt.button == BTN_ID_LEFT) {
            queueDirection(DIR_LEFT);
            return true;
        } else if (evt.button == BTN_ID_RIGHT) {
            queueDirection(DIR_RIGHT);
            return true;
        } else if (evt.button == BTN_ID_ENTER) {
            _state = SNAKE_STATE_PAUSED;
            return true;
        }
        return false;
    }

    // 3. In Paused State
    if (_state == SNAKE_STATE_PAUSED) {
        if (evt.button == BTN_ID_ENTER) {
            _state = SNAKE_STATE_PLAYING;
            return true;
        } else if (evt.button == BTN_ID_LEFT) {
            resetToMenu();
            return true;
        }
        return false;
    }

    // 4. In Game Over State
    if (_state == SNAKE_STATE_GAME_OVER) {
        if (evt.button == BTN_ID_ENTER) {
            startNewGame();
            return true;
        } else if (evt.button == BTN_ID_LEFT) {
            resetToMenu();
            return true;
        }
        return false;
    }

    return false;
}
