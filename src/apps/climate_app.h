#pragma once
#include "src/core/app_base.h"
#include "src/core/shared_data.h"

// ==============================================================================
// ORBIT v2 — Climate & Environment App (DHT11)
// ==============================================================================

#define CLIMATE_HISTORY_POINTS 24

class ClimateApp : public AppBase {
public:
    ClimateApp() = default;
    ~ClimateApp() override = default;

    AppMetadata getMetadata() const override {
        return { "climate", "Climate", 2, true }; // Background logging enabled
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;

    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;

    static void fetchLiveWeather();

private:
    void renderIndoorPage(RenderEngine& gfx, const TelemetryData& telem);
    void renderOutdoorPage(RenderEngine& gfx, const TelemetryData& telem);
    void renderComparePage(RenderEngine& gfx, const TelemetryData& telem);

    float _tempHistory[CLIMATE_HISTORY_POINTS] = {0};
    uint32_t _lastHistorySample = 0;
    uint32_t _lastWeatherFetch = 0;
    uint8_t _currentPage = 0; // 0 = Indoor, 1 = Outdoor, 2 = Delta Analysis
};
