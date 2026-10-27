#pragma once
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — Retro Arcade Snake Game
// Ported from Snake CLI Specification (All Modes, Power-Ups & Collision Physics)
// ==============================================================================

enum SnakeMode : uint8_t {
    SNAKE_MODE_CLASSIC = 0,
    SNAKE_MODE_TIME_ATTACK,
    SNAKE_MODE_SURVIVAL,
    SNAKE_MODE_ZEN,
    SNAKE_MODE_COUNT
};

enum SnakeState : uint8_t {
    SNAKE_STATE_MENU = 0,
    SNAKE_STATE_PLAYING,
    SNAKE_STATE_PAUSED,
    SNAKE_STATE_GAME_OVER
};

enum SnakeDirection : uint8_t {
    DIR_NONE = 0,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
};

enum SnakeItemType : uint8_t {
    ITEM_NONE = 0,
    ITEM_FOOD,      // Standard Apple (+10 pts / +25 in Time Attack, grows +1)
    ITEM_SLOW,      // Slow-Mo 5s (Halves engine speed, +20 pts)
    ITEM_GHOST,     // Ghost Mode 10s (Zen wrap-around, phases through self/obstacles, +20 pts)
    ITEM_3X,        // 3x Multiplier 5s (Triples all food points, +30 pts)
    ITEM_SHRINK,    // Shrink (Halves body length, minimum 3, +15 pts)
    ITEM_POISON     // Poison Hazard (-30 pts, +1 unrewarded length, 5s toxic speed surge)
};

enum SnakeDifficulty : uint8_t {
    DIFF_EASY = 0,
    DIFF_NORMAL,
    DIFF_HARD,
    DIFF_COUNT
};

struct SnakePoint {
    int8_t x;
    int8_t y;

    bool operator==(const SnakePoint& o) const {
        return x == o.x && y == o.y;
    }
};

class SnakeApp : public AppBase {
public:
    SnakeApp() = default;
    ~SnakeApp() override = default;

    AppMetadata getMetadata() const override {
        return { "snake", "Snake", 11, false };
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;

private:
    // Game Lifecycle
    void startNewGame();
    void resetToMenu();
    void tickGame();
    void spawnFood();
    void spawnSpecialItem();
    void generateObstacles();

    // Mechanics & Physics
    void handleItemPickup(SnakeItemType item);
    void triggerGameOver(const char* reason);
    bool isCellOccupied(int8_t x, int8_t y, bool includeSnake = true, bool includeObstacles = true) const;
    void queueDirection(SnakeDirection newDir);

    // Rendering Helpers
    void renderMenu(RenderEngine& gfx);
    void renderHUD(RenderEngine& gfx);
    void renderArena(RenderEngine& gfx);
    void renderSnake(RenderEngine& gfx);
    void renderItems(RenderEngine& gfx);
    void renderObstacles(RenderEngine& gfx);
    void renderPaused(RenderEngine& gfx);
    void renderGameOver(RenderEngine& gfx);

    // Persistence
    int getHighScore(SnakeMode mode);
    void saveHighScore(SnakeMode mode, int score);

    // Grid Dimensions
    static constexpr uint8_t GRID_W = 20;
    static constexpr uint8_t GRID_H = 22;
    static constexpr uint8_t CELL_SIZE = 6;
    static constexpr uint8_t ARENA_X = 4;
    static constexpr uint8_t ARENA_Y = 26;
    static constexpr uint16_t MAX_SNAKE_LEN = 250;
    static constexpr uint8_t MAX_OBSTACLES = 40;

    // Game State
    SnakeState _state = SNAKE_STATE_MENU;
    SnakeMode _selectedMode = SNAKE_MODE_CLASSIC;
    SnakeDifficulty _difficulty = DIFF_NORMAL;
    bool _obstaclesEnabled = false;

    // Active Run Variables
    SnakePoint _body[MAX_SNAKE_LEN];
    uint16_t _length = 3;
    bool _growing = false;
    int _score = 0;
    int _highScore = 0;
    bool _newHighScore = false;
    const char* _deathReason = "";

    // 2-Event Direction Queue (Anti-Suicide Protection)
    SnakeDirection _dir = DIR_RIGHT;
    SnakeDirection _dirQueue[2];
    uint8_t _queueLen = 0;

    // Timing & Physics
    uint32_t _tickTimer = 0;
    uint32_t _baseTickMs = 120;
    uint32_t _currentTickMs = 120;

    // Time Attack Mode Clock
    float _timeAttackRemaining = 60.0f; // seconds

    // Items & Power-Ups
    SnakePoint _foodPos = { 10, 11 };
    SnakePoint _specialPos = { -1, -1 };
    SnakeItemType _specialType = ITEM_NONE;
    uint32_t _specialSpawnTimer = 0;
    uint32_t _specialDespawnTimer = 0;

    // Active Power-Up Effects
    float _ghostTimer = 0.0f;       // 10s Zen mode
    float _slowTimer = 0.0f;        // 5s Half speed
    float _multiplierTimer = 0.0f;  // 5s 3x Points
    float _poisonTimer = 0.0f;      // 5s Double speed hazard

    // Obstacles
    SnakePoint _obstacles[MAX_OBSTACLES];
    uint8_t _numObstacles = 0;

    // Menu Navigation
    uint8_t _menuRow = 0; // 0: Mode, 1: Obstacles, 2: Difficulty, 3: Start
};
