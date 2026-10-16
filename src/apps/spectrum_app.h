#pragma once
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — Real-Time 16-Band Audio Spectrum & VU Visualizer
// ==============================================================================

enum SpectrumMode {
    SPEC_BARS_16 = 0,
    SPEC_ANALOG_VU,
    SPEC_WAVEFORM,
    SPEC_MODE_MAX
};

class SpectrumApp : public AppBase {
public:
    SpectrumApp() = default;
    ~SpectrumApp() override = default;

    AppMetadata getMetadata() const override {
        return { "spectrum", "Spectrum", 2, false };
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;

private:
    void renderBars(RenderEngine& gfx);
    void renderVuMeter(RenderEngine& gfx);
    void renderWaveform(RenderEngine& gfx);

    SpectrumMode _mode = SPEC_BARS_16;
    uint8_t _peakHold[FFT_NUM_BINS] = {0};
    uint32_t _lastPeakDecay = 0;
    float _vuNeedleAngle = -0.785f; // -45 deg
};
