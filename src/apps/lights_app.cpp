#include "src/apps/lights_app.h"
#include "src/core/app_registry.h"
#include "src/lighting/rgb_controller.h"
#include "src/display/theme.h"

REGISTER_APP(LightsApp);

void LightsApp::onInit() {}

void LightsApp::onEnter() {
    RgbLightingMode mode = RgbController::getInstance().getMode();
    if (mode != RGB_OFF) {
        _lastActiveMode = mode;
    }
}

void LightsApp::onExit() {}
void LightsApp::onUpdate(uint32_t deltaMs) {}

void LightsApp::onRender(RenderEngine& gfx) {
    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);
    gfx.drawHeader("LIGHTS & RGB", 7);

    RgbLightingMode mode = RgbController::getInstance().getMode();
    uint8_t brightness = RgbController::getInstance().getBrightness();

    if (mode != RGB_SOLID && _selectedField > 1) {
        _selectedField = 1;
    }

    char bBuf[16];
    uint8_t pct = (brightness * 100) / 255;
    snprintf(bBuf, sizeof(bBuf), "%d%%", pct);

    if (mode != RGB_SOLID) {
        // --------------------------------------------------------------------------
        // 2-Card Layout (Standard Modes & Off)
        // --------------------------------------------------------------------------
        // Card 0: Mode & Power (x=8, y=39, w=112, h=48)
        uint16_t b0Color = (_selectedField == 0) ? COLOR_NEON_CYAN : COLOR_SLATE_BORDER;
        gfx.drawCard(8, 39, 112, 48, COLOR_SLATE_MID, b0Color);

        // Header Line: Left label (x=14) and Right Status Pill (xRight=114) -> 35px clear margin!
        const char* t0 = (_selectedField == 0) ? "> MODE" : "  MODE";
        gfx.drawText(14, 44, t0, (_selectedField == 0) ? COLOR_NEON_CYAN : COLOR_TEXT_MUTED, 1);

        if (mode == RGB_OFF) {
            gfx.drawTextRight(114, 44, "[OFF]", COLOR_TEXT_MUTED, 1);
        } else {
            char idxBuf[16];
            snprintf(idxBuf, sizeof(idxBuf), "%02d/10 [ON]", (uint8_t)mode);
            gfx.drawTextRight(114, 44, idxBuf, COLOR_NEON_MINT, 1);
        }

        const char* modeName = "LIGHT IS OFF";
        uint16_t modeColor = COLOR_TEXT_MUTED;
        switch (mode) {
            case RGB_OFF:         modeName = "LIGHT IS OFF"; modeColor = COLOR_TEXT_MUTED; break;
            case RGB_AMBIENT:     modeName = "AMBIENT CLIMATE"; modeColor = COLOR_NEON_MINT; break;
            case RGB_DISCO:       modeName = "DISCO (SOUND)"; modeColor = COLOR_NEON_AMBER; break;
            case RGB_BREATHE:     modeName = "PULSE BREATHE"; modeColor = COLOR_NEON_PURPLE; break;
            case RGB_RAINBOW:     modeName = "RAINBOW CYCLE"; modeColor = COLOR_NEON_CYAN; break;
            case RGB_ALERT:       modeName = "CRITICAL ALERT"; modeColor = COLOR_NEON_RED; break;
            case RGB_DISCO_ALERT: modeName = "DISCO STROBE"; modeColor = COLOR_NEON_RED; break;
            case RGB_CANDLE:      modeName = "WARM CANDLE"; modeColor = COLOR_NEON_AMBER; break;
            case RGB_AURORA:      modeName = "AURORA LIGHTS"; modeColor = COLOR_NEON_MINT; break;
            case RGB_POLICE:      modeName = "POLICE SIREN"; modeColor = COLOR_NEON_CYAN; break;
            default: break;
        }
        gfx.drawTextCentered(64, 58, modeName, modeColor, 1);

        if (_selectedField == 0) {
            gfx.drawTextCentered(64, 72, "< PREV / NEXT >", COLOR_TEXT_MUTED, 1);
        }

        // Card 1: Brightness Level (x=8, y=91, w=112, h=45)
        uint16_t b1Color = (_selectedField == 1) ? COLOR_NEON_CYAN : COLOR_SLATE_BORDER;
        gfx.drawCard(8, 91, 112, 45, COLOR_SLATE_MID, b1Color);

        const char* t1 = (_selectedField == 1) ? "> RGB LEVEL" : "  RGB LEVEL";
        gfx.drawText(14, 96, t1, (_selectedField == 1) ? COLOR_NEON_CYAN : COLOR_TEXT_MUTED, 1);
        gfx.drawTextRight(114, 96, bBuf, (mode == RGB_OFF) ? COLOR_TEXT_MUTED : COLOR_TEXT_LIGHT, 1);

        gfx.drawProgressBar(14, 110, 100, 5, pct, (mode == RGB_OFF) ? COLOR_TEXT_MUTED : COLOR_NEON_CYAN);

        if (_selectedField == 1) {
            gfx.drawTextCentered(64, 123, "< DIM / BOOST >", COLOR_TEXT_MUTED, 1);
        }

        // Bottom hint (y=146, 19 chars max = 114px, 7px side margins)
        if (_selectedField == 0) {
            gfx.drawTextCentered(64, 146, "OK:Power  L/R:Mode", COLOR_TEXT_MUTED, 1);
        } else {
            gfx.drawTextCentered(64, 146, "L/R:Dim   OK:Jump", COLOR_TEXT_MUTED, 1);
        }
    } else {
        // --------------------------------------------------------------------------
        // Custom RGB Studio Layout (Mode + Live Swatch + 4 Channel Sliders)
        // --------------------------------------------------------------------------
        uint32_t currColor = RgbController::getInstance().getColor();
        uint8_t r = (currColor >> 16) & 0xFF;
        uint8_t g = (currColor >> 8) & 0xFF;
        uint8_t b = currColor & 0xFF;
        uint16_t preview565 = RGB565(r, g, b);

        // Card 0: Mode & Live Color Preview Swatch (y=39, h=26 -> ends at 65)
        uint16_t b0Color = (_selectedField == 0) ? COLOR_NEON_CYAN : COLOR_SLATE_BORDER;
        gfx.drawCard(8, 39, 112, 26, COLOR_SLATE_MID, b0Color);

        const char* t0 = (_selectedField == 0) ? "> MODE" : "  MODE";
        gfx.drawText(12, 43, t0, (_selectedField == 0) ? COLOR_NEON_CYAN : COLOR_TEXT_MUTED, 1);
        gfx.drawText(14, 52, "CUSTOM SOLID", COLOR_WHITE, 1);

        // Live Color Preview Pill Swatch (x=82, y=42, w=32, h=19)
        gfx.fillRoundRect(82, 42, 32, 19, 2, preview565);
        gfx.drawRoundRect(82, 42, 32, 19, 2, COLOR_WHITE);

        // Card 1: Studio Mixer Sliders (y=67, h=71 -> ends at 138)
        gfx.drawCard(8, 67, 112, 71, COLOR_SLATE_MID, COLOR_SLATE_BORDER);

        // Row 0: Brightness (Field 1)
        {
            bool isSel = (_selectedField == 1);
            uint16_t accent = COLOR_NEON_CYAN;
            if (isSel) {
                gfx.drawFastVLine(9, 71, 8, accent);
                gfx.drawFastVLine(10, 71, 8, accent);
                gfx.drawText(11, 71, ">", COLOR_WHITE, 1);
            }
            gfx.drawText(16, 71, "BRT", isSel ? COLOR_WHITE : COLOR_TEXT_MUTED, 1);
            gfx.drawTextRight(56, 71, bBuf, isSel ? COLOR_WHITE : COLOR_TEXT_LIGHT, 1);
            gfx.drawProgressBar(60, 72, 52, 6, pct, COLOR_NEON_CYAN, COLOR_SLATE_DARK);
            if (isSel) {
                gfx.drawRoundRect(59, 71, 54, 8, 3, accent);
            }
        }

        // Row 1: Red (Field 2)
        {
            bool isSel = (_selectedField == 2);
            uint16_t accent = COLOR_NEON_RED;
            if (isSel) {
                gfx.drawFastVLine(9, 87, 8, accent);
                gfx.drawFastVLine(10, 87, 8, accent);
                gfx.drawText(11, 87, ">", COLOR_WHITE, 1);
            }
            char rBuf[8];
            snprintf(rBuf, sizeof(rBuf), "%d", r);
            uint8_t rPct = (uint16_t)r * 100 / 255;
            gfx.drawText(16, 87, "RED", isSel ? COLOR_NEON_RED : COLOR_TEXT_MUTED, 1);
            gfx.drawTextRight(56, 87, rBuf, isSel ? COLOR_WHITE : COLOR_TEXT_LIGHT, 1);
            gfx.drawProgressBar(60, 88, 52, 6, rPct, COLOR_NEON_RED, COLOR_SLATE_DARK);
            if (isSel) {
                gfx.drawRoundRect(59, 87, 54, 8, 3, accent);
            }
        }

        // Row 2: Green (Field 3)
        {
            bool isSel = (_selectedField == 3);
            uint16_t accent = COLOR_NEON_MINT;
            if (isSel) {
                gfx.drawFastVLine(9, 103, 8, accent);
                gfx.drawFastVLine(10, 103, 8, accent);
                gfx.drawText(11, 103, ">", COLOR_WHITE, 1);
            }
            char gBuf[8];
            snprintf(gBuf, sizeof(gBuf), "%d", g);
            uint8_t gPct = (uint16_t)g * 100 / 255;
            gfx.drawText(16, 103, "GRN", isSel ? COLOR_NEON_MINT : COLOR_TEXT_MUTED, 1);
            gfx.drawTextRight(56, 103, gBuf, isSel ? COLOR_WHITE : COLOR_TEXT_LIGHT, 1);
            gfx.drawProgressBar(60, 104, 52, 6, gPct, COLOR_NEON_MINT, COLOR_SLATE_DARK);
            if (isSel) {
                gfx.drawRoundRect(59, 103, 54, 8, 3, accent);
            }
        }

        // Row 3: Blue (Field 4)
        {
            bool isSel = (_selectedField == 4);
            uint16_t accent = COLOR_NEON_CYAN;
            if (isSel) {
                gfx.drawFastVLine(9, 119, 8, accent);
                gfx.drawFastVLine(10, 119, 8, accent);
                gfx.drawText(11, 119, ">", COLOR_WHITE, 1);
            }
            char bValBuf[8];
            snprintf(bValBuf, sizeof(bValBuf), "%d", b);
            uint8_t bPct = (uint16_t)b * 100 / 255;
            gfx.drawText(16, 119, "BLU", isSel ? COLOR_NEON_CYAN : COLOR_TEXT_MUTED, 1);
            gfx.drawTextRight(56, 119, bValBuf, isSel ? COLOR_WHITE : COLOR_TEXT_LIGHT, 1);
            gfx.drawProgressBar(60, 120, 52, 6, bPct, COLOR_NEON_CYAN, COLOR_SLATE_DARK);
            if (isSel) {
                gfx.drawRoundRect(59, 119, 54, 8, 3, accent);
            }
        }

        // Bottom hint (y=146, 19 chars max = 114px, 7px side margins)
        if (_selectedField == 0) {
            gfx.drawTextCentered(64, 146, "OK:Power  L/R:Mode", COLOR_TEXT_MUTED, 1);
        } else if (_selectedField == 1) {
            gfx.drawTextCentered(64, 146, "L/R:Dim   OK:Jump", COLOR_TEXT_MUTED, 1);
        } else if (_selectedField == 2) {
            gfx.drawTextCentered(64, 146, "L/R:Red   OK:Toggle", COLOR_TEXT_MUTED, 1);
        } else if (_selectedField == 3) {
            gfx.drawTextCentered(64, 146, "L/R:Green OK:Toggle", COLOR_TEXT_MUTED, 1);
        } else {
            gfx.drawTextCentered(64, 146, "L/R:Blue  OK:Toggle", COLOR_TEXT_MUTED, 1);
        }
    }
}

bool LightsApp::onButtonEvent(const ButtonEvent& evt) {
    // 1s Enter = 1 step back
    if (evt.button == BTN_ID_ENTER && (evt.type == BUTTON_HOLD_1S || evt.type == BUTTON_HOLD_05S)) {
        if (_selectedField > 0) {
            _selectedField = 0; // 1 step back to Mode & Power control
            RenderEngine::getInstance().showToast("BACK", "Mode Select", COLOR_TEXT_MUTED, 800);
            return true;
        }
        return false; // On Field 0: allow AppRegistry to step back out to Launcher Menu
    }

    if (evt.type == BUTTON_CLICK || evt.type == BUTTON_REPEAT) {
        RgbLightingMode currentMode = RgbController::getInstance().getMode();
        uint8_t maxFields = (currentMode == RGB_SOLID) ? 5 : 2;

        if (evt.button == BTN_ID_UP) {
            _selectedField = (_selectedField == 0) ? (maxFields - 1) : (_selectedField - 1);
            return true;
        } else if (evt.button == BTN_ID_DOWN) {
            _selectedField = (_selectedField + 1) % maxFields;
            return true;
        }

        if (_selectedField == 0) {
            // Mode & Power
            if (evt.button == BTN_ID_ENTER) {
                // Quick Power Toggle
                if (currentMode == RGB_OFF) {
                    RgbLightingMode restored = (_lastActiveMode != RGB_OFF) ? _lastActiveMode : RGB_AMBIENT;
                    RgbController::getInstance().setMode(restored);
                } else {
                    _lastActiveMode = currentMode;
                    RgbController::getInstance().setMode(RGB_OFF);
                }
                return true;
            } else if (evt.button == BTN_ID_RIGHT) {
                uint8_t next = (currentMode + 1) % RGB_MODE_MAX;
                RgbController::getInstance().setMode(static_cast<RgbLightingMode>(next));
                if (next != RGB_OFF) _lastActiveMode = static_cast<RgbLightingMode>(next);
                return true;
            } else if (evt.button == BTN_ID_LEFT) {
                uint8_t prev = (currentMode == 0) ? (RGB_MODE_MAX - 1) : (currentMode - 1);
                RgbController::getInstance().setMode(static_cast<RgbLightingMode>(prev));
                if (prev != RGB_OFF) _lastActiveMode = static_cast<RgbLightingMode>(prev);
                return true;
            }
        } else if (_selectedField == 1) {
            // Brightness
            uint8_t b = RgbController::getInstance().getBrightness();
            if (evt.button == BTN_ID_RIGHT) {
                b = (b <= 230) ? (b + 25) : 255;
                RgbController::getInstance().setBrightness(b);
                return true;
            } else if (evt.button == BTN_ID_LEFT) {
                b = (b >= 25) ? (b - 25) : 0;
                RgbController::getInstance().setBrightness(b);
                return true;
            } else if (evt.button == BTN_ID_ENTER) {
                // Preset jumps: 64 (25%) -> 128 (50%) -> 255 (100%)
                if (b < 100) b = 128;
                else if (b < 200) b = 255;
                else b = 64;
                RgbController::getInstance().setBrightness(b);
                return true;
            }
        } else if (_selectedField == 2) {
            // Red Channel (0-255)
            uint32_t c = RgbController::getInstance().getColor();
            uint8_t r = (c >> 16) & 0xFF;
            uint8_t g = (c >> 8) & 0xFF;
            uint8_t b = c & 0xFF;

            if (evt.button == BTN_ID_RIGHT) {
                r = (r <= 240) ? (r + 15) : 255;
                RgbController::getInstance().setColor(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
                return true;
            } else if (evt.button == BTN_ID_LEFT) {
                r = (r >= 15) ? (r - 15) : 0;
                RgbController::getInstance().setColor(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
                return true;
            } else if (evt.button == BTN_ID_ENTER) {
                // Quick toggle: 0 -> 128 -> 255 -> 0
                if (r == 0) r = 128;
                else if (r < 255) r = 255;
                else r = 0;
                RgbController::getInstance().setColor(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
                return true;
            }
        } else if (_selectedField == 3) {
            // Green Channel (0-255)
            uint32_t c = RgbController::getInstance().getColor();
            uint8_t r = (c >> 16) & 0xFF;
            uint8_t g = (c >> 8) & 0xFF;
            uint8_t b = c & 0xFF;

            if (evt.button == BTN_ID_RIGHT) {
                g = (g <= 240) ? (g + 15) : 255;
                RgbController::getInstance().setColor(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
                return true;
            } else if (evt.button == BTN_ID_LEFT) {
                g = (g >= 15) ? (g - 15) : 0;
                RgbController::getInstance().setColor(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
                return true;
            } else if (evt.button == BTN_ID_ENTER) {
                // Quick toggle: 0 -> 128 -> 255 -> 0
                if (g == 0) g = 128;
                else if (g < 255) g = 255;
                else g = 0;
                RgbController::getInstance().setColor(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
                return true;
            }
        } else if (_selectedField == 4) {
            // Blue Channel (0-255)
            uint32_t c = RgbController::getInstance().getColor();
            uint8_t r = (c >> 16) & 0xFF;
            uint8_t g = (c >> 8) & 0xFF;
            uint8_t b = c & 0xFF;

            if (evt.button == BTN_ID_RIGHT) {
                b = (b <= 240) ? (b + 15) : 255;
                RgbController::getInstance().setColor(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
                return true;
            } else if (evt.button == BTN_ID_LEFT) {
                b = (b >= 15) ? (b - 15) : 0;
                RgbController::getInstance().setColor(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
                return true;
            } else if (evt.button == BTN_ID_ENTER) {
                // Quick toggle: 0 -> 128 -> 255 -> 0
                if (b == 0) b = 128;
                else if (b < 255) b = 255;
                else b = 0;
                RgbController::getInstance().setColor(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
                return true;
            }
        }
    }
    return false;
}
