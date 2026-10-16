#pragma once
#include <Arduino.h>

// ==============================================================================
// ORBIT v2 — Visual Theme & 16-Bit RGB565 Color Palette
// ==============================================================================

// Helper macro for RGB888 to RGB565 conversion
#define RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

// Core Color Palette (RGB565)
#define COLOR_BLACK           0x0000
#define COLOR_WHITE           0xFFFF
#define COLOR_VOID_DARK       0x0842  // Very deep slate black
#define COLOR_SLATE_DARK      0x18E3  // Panel background
#define COLOR_SLATE_MID       0x2965  // Card background
#define COLOR_SLATE_BORDER    0x4A69  // Border / Divider line

// Vibrant Neon Accents
#define COLOR_NEON_CYAN       0x07FF  // Primary highlight
#define COLOR_NEON_PURPLE     0xB81F  // Secondary highlight
#define COLOR_NEON_MINT       0x2FE8  // Energy / Active
#define COLOR_NEON_AMBER      0xFD20  // Warnings / Solar
#define COLOR_NEON_CORAL      0xFA60  // Accent / Heat
#define COLOR_NEON_RED        0xF800  // Alerts / Critical
#define COLOR_NEON_BLUE       0x34DF  // Secondary blue

// Grayscale & Text
#define COLOR_TEXT_LIGHT      0xFFFF
#define COLOR_TEXT_MUTED      0x9CD3
#define COLOR_TEXT_DARK       0x52AA

// Theme Presets
enum ThemeMode {
    THEME_CYBER_NEON = 0,
    THEME_SOLAR_AMBER,
    THEME_MATRIX_EMERALD,
    THEME_ASTRAL_DEEP,
    THEME_RETRO_MONO,
    THEME_COUNT
};

struct ThemeColors {
    uint16_t bg;
    uint16_t cardBg;
    uint16_t border;
    uint16_t primary;
    uint16_t secondary;
    uint16_t accent;
    uint16_t text;
    uint16_t textMuted;
    uint16_t alert;
};

// Default Theme: Cyber Neon
static const ThemeColors THEMES[THEME_COUNT] = {
    // 0: Cyber Neon
    {
        COLOR_VOID_DARK,     // bg
        COLOR_SLATE_MID,     // cardBg
        COLOR_SLATE_BORDER,  // border
        COLOR_NEON_CYAN,     // primary
        COLOR_NEON_PURPLE,   // secondary
        COLOR_NEON_MINT,     // accent
        COLOR_TEXT_LIGHT,    // text
        COLOR_TEXT_MUTED,    // textMuted
        COLOR_NEON_RED       // alert
    },
    // 1: Solar Amber
    {
        0x0820,              // bg (deep warm brown)
        0x2100,              // cardBg
        0x5A80,              // border
        0xFEA0,              // primary (gold)
        0xFD20,              // secondary (amber)
        0xFBE0,              // accent
        COLOR_TEXT_LIGHT,    // text
        0xC580,              // textMuted
        COLOR_NEON_RED       // alert
    },
    // 2: Matrix Emerald
    {
        0x0100,              // bg (deep black-green)
        0x0260,              // cardBg
        0x04C0,              // border
        0x07E0,              // primary (green)
        0x3FE0,              // secondary
        0x57E0,              // accent
        COLOR_TEXT_LIGHT,    // text
        0x04A0,              // textMuted
        COLOR_NEON_AMBER     // alert
    },
    // 3: Astral Deep
    {
        0x0008,              // bg (deep astral navy)
        0x10B0,              // cardBg
        0x29D8,              // border
        0x3DFE,              // primary (ice blue)
        0x721F,              // secondary (lavender)
        COLOR_NEON_CYAN,     // accent
        COLOR_TEXT_LIGHT,    // text
        0x8455,              // textMuted
        COLOR_NEON_CORAL     // alert
    },
    // 4: Retro Monochrome
    {
        0x0000,              // bg
        0x2104,              // cardBg
        0x632C,              // border
        0xFFFF,              // primary
        0xCE79,              // secondary
        0x8410,              // accent
        0xFFFF,              // text
        0x8410,              // textMuted
        0xFFFF               // alert
    }
};

// Fast Theme Access Macro
#define THEME_BG          THEMES[0].bg
#define THEME_CARD_BG     THEMES[0].cardBg
#define THEME_BORDER      THEMES[0].border
#define THEME_PRIMARY     THEMES[0].primary
#define THEME_SECONDARY   THEMES[0].secondary
#define THEME_ACCENT      THEMES[0].accent
#define THEME_TEXT        THEMES[0].text
#define THEME_TEXT_MUTED  THEMES[0].textMuted
#define THEME_ALERT       THEMES[0].alert
