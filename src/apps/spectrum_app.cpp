#include "src/apps/spectrum_app.h"
#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/display/theme.h"
#include <cmath>

REGISTER_APP(SpectrumApp);

void SpectrumApp::onInit() {}
void SpectrumApp::onEnter() {
    memset(_peakHold, 0, sizeof(_peakHold));
}
void SpectrumApp::onExit() {}

void SpectrumApp::onUpdate(uint32_t deltaMs) {
    uint32_t now = millis();
    if (now - _lastPeakDecay > 60) {
        _lastPeakDecay = now;
        for (int i = 0; i < FFT_NUM_BINS; ++i) {
            if (_peakHold[i] > 2) _peakHold[i] -= 2;
        }
    }

    TelemetryData telem = SharedData::getInstance().getData();
    // Ballistic VU needle damping
    float targetAngle = -0.785f + (telem.micDb - 35.0f) / 60.0f * 1.57f;
    targetAngle = constrain(targetAngle, -0.785f, 0.785f);
    _vuNeedleAngle = (_vuNeedleAngle * 0.7f) + (targetAngle * 0.3f);
}

void SpectrumApp::onRender(RenderEngine& gfx) {
    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);

    const char* headerTitle = "SPECTRUM (EQ)";
    if (_mode == SPEC_ANALOG_VU) headerTitle = "VU BALLISTICS";
    else if (_mode == SPEC_WAVEFORM) headerTitle = "OSCILLOSCOPE";
    gfx.drawHeader(headerTitle, 2);

    if (_mode == SPEC_BARS_16) {
        renderBars(gfx);
    } else if (_mode == SPEC_ANALOG_VU) {
        renderVuMeter(gfx);
    } else {
        renderWaveform(gfx);
    }

    // Bottom Mode Switcher Hint
    gfx.drawTextCentered(64, SCREEN_HEIGHT - 12, "SELECT: Switch Mode", COLOR_TEXT_MUTED, 1);
}

void SpectrumApp::renderBars(RenderEngine& gfx) {
    TelemetryData telem = SharedData::getInstance().getData();

    int16_t startX = 8;
    int16_t barWidth = 5;
    int16_t barSpacing = 2;
    int16_t maxBarH = 75;
    int16_t baseY = STATUS_BAR_HEIGHT + 105;

    // Draw baseline
    gfx.drawFastHLine(startX - 2, baseY + 1, (barWidth + barSpacing) * FFT_NUM_BINS + 2, COLOR_SLATE_BORDER);

    for (int i = 0; i < FFT_NUM_BINS; ++i) {
        uint8_t val = telem.fftBands[i];
        if (val > _peakHold[i]) {
            _peakHold[i] = val;
        }

        int16_t h = map(val, 0, 255, 0, maxBarH);
        int16_t peakH = map(_peakHold[i], 0, 255, 0, maxBarH);

        int16_t bx = startX + i * (barWidth + barSpacing);

        // Gradient color: Cyan at bottom, Purple in mid, Amber/Red at top
        uint16_t barColor = COLOR_NEON_CYAN;
        if (h > 50) barColor = COLOR_NEON_PURPLE;
        if (h > 65) barColor = COLOR_NEON_AMBER;

        if (h > 0) {
            gfx.fillRect(bx, baseY - h, barWidth, h, barColor);
        }

        // Draw Peak Marker dot
        if (peakH > 0) {
            gfx.drawFastHLine(bx, baseY - peakH, barWidth, COLOR_WHITE);
        }
    }

    // dB label below bars
    char dbBuf[16];
    snprintf(dbBuf, sizeof(dbBuf), "%.1f dB SPL", telem.micDb);
    gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 112, dbBuf, COLOR_TEXT_LIGHT, 1);
}

void SpectrumApp::renderVuMeter(RenderEngine& gfx) {
    int16_t cx = 64;
    int16_t cy = STATUS_BAR_HEIGHT + 85;
    int16_t radius = 55;

    // Outer Dial Arc
    gfx.drawCircle(cx, cy, radius, COLOR_SLATE_BORDER);
    gfx.drawCircle(cx, cy, radius - 1, COLOR_SLATE_MID);

    // Color arcs: Green (safe), Amber (loud), Red (clip)
    // Ticks
    for (float a = -0.785f; a <= 0.785f; a += 0.26f) {
        int16_t x0 = cx + (int16_t)((radius - 6) * sin(a));
        int16_t y0 = cy - (int16_t)((radius - 6) * cos(a));
        int16_t x1 = cx + (int16_t)(radius * sin(a));
        int16_t y1 = cy - (int16_t)(radius * cos(a));
        uint16_t tColor = (a > 0.5f) ? COLOR_NEON_RED : (a > 0.2f ? COLOR_NEON_AMBER : COLOR_NEON_MINT);
        gfx.drawLine(x0, y0, x1, y1, tColor);
    }

    // Dial Needle
    int16_t nx = cx + (int16_t)((radius - 4) * sin(_vuNeedleAngle));
    int16_t ny = cy - (int16_t)((radius - 4) * cos(_vuNeedleAngle));
    gfx.drawLine(cx, cy, nx, ny, COLOR_NEON_CYAN);
    gfx.fillCircle(cx, cy, 4, COLOR_WHITE);

    // Reading
    TelemetryData telem = SharedData::getInstance().getData();
    char dbBuf[16];
    snprintf(dbBuf, sizeof(dbBuf), "%.1f dB", telem.micDb);
    gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 96, dbBuf, COLOR_TEXT_LIGHT, 1);
}

void SpectrumApp::renderWaveform(RenderEngine& gfx) {
    TelemetryData telem = SharedData::getInstance().getData();
    int16_t cy = STATUS_BAR_HEIGHT + 65;

    gfx.drawFastHLine(6, cy, SCREEN_WIDTH - 12, COLOR_SLATE_BORDER);

    // Reconstruct pseudo-waveform from FFT energy
    int16_t prevX = 8;
    int16_t prevY = cy;

    for (int i = 0; i < 20; ++i) {
        int16_t x = 8 + i * 5;
        float phase = (millis() * 0.01f) + (i * 0.4f);
        int16_t amp = (int16_t)((telem.micDb - 35.0f) * 0.5f * sin(phase));
        int16_t y = cy + amp;
        gfx.drawLine(prevX, prevY, x, y, COLOR_NEON_MINT);
        prevX = x;
        prevY = y;
    }

    char dbBuf[16];
    snprintf(dbBuf, sizeof(dbBuf), "Live RMS: %.0f", telem.micRms);
    gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 104, dbBuf, COLOR_TEXT_MUTED, 1);
}

bool SpectrumApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK && (evt.button == BTN_ID_ENTER || evt.button == BTN_ID_SELECT)) {
        _mode = static_cast<SpectrumMode>((_mode + 1) % SPEC_MODE_MAX);
        return true;
    }
    return false;
}
