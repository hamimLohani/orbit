#include "src/apps/climate_app.h"
#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/display/theme.h"
#include "src/display/render_engine.h"
#include "src/network/wifi_manager.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

REGISTER_APP(ClimateApp);

void ClimateApp::onInit() {
    for (int i = 0; i < CLIMATE_HISTORY_POINTS; ++i) {
        _tempHistory[i] = 22.0f;
    }
}

void ClimateApp::onEnter() {
    uint32_t now = millis();
    if (_lastWeatherFetch == 0 || now - _lastWeatherFetch > 600000) {
        fetchLiveWeather();
        _lastWeatherFetch = now;
    }
}

void ClimateApp::onExit() {}

void ClimateApp::onUpdate(uint32_t deltaMs) {
    uint32_t now = millis();

    // 1. Record indoor temperature history every 30s
    if (now - _lastHistorySample > 30000) {
        _lastHistorySample = now;
        TelemetryData telem = SharedData::getInstance().getData();

        for (int i = 0; i < CLIMATE_HISTORY_POINTS - 1; ++i) {
            _tempHistory[i] = _tempHistory[i + 1];
        }
        _tempHistory[CLIMATE_HISTORY_POINTS - 1] = telem.temperature;
    }

    // 2. Refresh live weather in background every 10 minutes if Wi-Fi connected
    if (_lastWeatherFetch == 0 || now - _lastWeatherFetch > 600000) {
        if (SharedData::getInstance().getData().wifiConnected) {
            fetchLiveWeather();
            _lastWeatherFetch = now;
        }
    }
}

void ClimateApp::fetchLiveWeather() {
    TelemetryData telem = SharedData::getInstance().getData();
    if (!telem.wifiConnected) return;
    if (strlen(DEFAULT_OPENWEATHER_API_KEY) < 10) return;

    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + String(DEFAULT_WEATHER_CITY) + "&units=metric&appid=" + String(DEFAULT_OPENWEATHER_API_KEY);
    http.begin(url);
    http.setTimeout(4500);

    int httpCode = http.GET();
    if (httpCode == 200) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (!err) {
            float temp = doc["main"]["temp"] | 21.0f;
            float feels = doc["main"]["feels_like"] | temp;
            float hum = doc["main"]["humidity"] | 50.0f;
            uint16_t press = doc["main"]["pressure"] | 1013;
            float wind = doc["wind"]["speed"] | 3.0f;
            const char* cond = doc["weather"][0]["main"] | "Clear";
            const char* desc = doc["weather"][0]["description"] | "Clear sky";
            const char* name = doc["name"] | DEFAULT_WEATHER_CITY;

            SharedData::getInstance().setWeather(temp, feels, hum, wind, press, cond, name, desc);
        }
    }
    http.end();
}

void ClimateApp::onRender(RenderEngine& gfx) {
    TelemetryData telem = SharedData::getInstance().getData();

    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);

    if (_currentPage == 0) {
        renderIndoorPage(gfx, telem);
    } else if (_currentPage == 1) {
        renderOutdoorPage(gfx, telem);
    } else {
        renderComparePage(gfx, telem);
    }
}

// ------------------------------------------------------------------------------
// Page 1: Indoor Microclimate (DHT11 Sensor)
// ------------------------------------------------------------------------------
void ClimateApp::renderIndoorPage(RenderEngine& gfx, const TelemetryData& telem) {
    gfx.drawHeader("INDOOR (1/3)", 2);

    // 1. Twin Metric Cards: Temp & Humidity (y=39..83, h=44)
    gfx.drawCard(6, 39, 56, 44, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(10, 43, "INDOOR", COLOR_TEXT_MUTED, 1);
    char tBuf[12];
    snprintf(tBuf, sizeof(tBuf), "%.1f", telem.temperature);
    gfx.drawText(10, 54, tBuf, COLOR_NEON_CYAN, 2);
    gfx.drawText(46, 43, "C", COLOR_TEXT_LIGHT, 1);
    gfx.drawPixel(42, 43, COLOR_NEON_CYAN);

    gfx.drawCard(66, 39, 56, 44, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(70, 43, "HUMIDITY", COLOR_TEXT_MUTED, 1);
    char hBuf[12];
    snprintf(hBuf, sizeof(hBuf), "%.0f%%", telem.humidity);
    gfx.drawText(70, 54, hBuf, COLOR_NEON_MINT, 2);
    uint8_t humPct = (uint8_t)constrain((int)telem.humidity, 0, 100);
    gfx.drawProgressBar(70, 74, 48, 3, humPct, COLOR_NEON_MINT);

    // 2. Comfort Pill Card (y=86..106, h=20)
    uint16_t comfortColor = COLOR_NEON_MINT;
    const char* comfortName = "IDEAL";
    if (telem.humidity > 70.0f) {
        comfortColor = COLOR_NEON_PURPLE;
        comfortName = "HUMID";
    } else if (telem.humidity < 30.0f) {
        comfortColor = COLOR_NEON_AMBER;
        comfortName = "DRY";
    } else if (telem.temperature > 28.0f) {
        comfortColor = COLOR_NEON_CORAL;
        comfortName = "WARM";
    } else if (telem.temperature < 19.0f) {
        comfortColor = COLOR_NEON_BLUE;
        comfortName = "COOL";
    }

    gfx.drawCard(6, 86, 116, 20, COLOR_SLATE_MID, comfortColor);
    gfx.fillCircle(14, 96, 3, comfortColor);
    gfx.drawText(22, 92, comfortName, comfortColor, 1);

    char heatBuf[16];
    snprintf(heatBuf, sizeof(heatBuf), "HI: %.1fC", telem.heatIndex);
    gfx.drawTextRight(118, 92, heatBuf, COLOR_TEXT_LIGHT, 1);

    // 3. 24-Sample Temperature Trend Card (y=109..142, h=33)
    gfx.drawCard(6, 109, 116, 33, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(10, 113, "24H TREND", COLOR_TEXT_MUTED, 1);

    float minT = _tempHistory[0];
    float maxT = _tempHistory[0];
    for (int i = 1; i < CLIMATE_HISTORY_POINTS; ++i) {
        if (_tempHistory[i] < minT) minT = _tempHistory[i];
        if (_tempHistory[i] > maxT) maxT = _tempHistory[i];
    }
    if (maxT - minT < 2.0f) {
        minT -= 1.0f;
        maxT += 1.0f;
    }

    char rangeBuf[16];
    snprintf(rangeBuf, sizeof(rangeBuf), "%.0f-%.0fC", minT, maxT);
    gfx.drawTextRight(118, 113, rangeBuf, COLOR_TEXT_LIGHT, 1);
    gfx.drawSparkline(10, 124, 108, 14, _tempHistory, CLIMATE_HISTORY_POINTS, minT, maxT, COLOR_NEON_CYAN);

    // 4. Navigation Hint (y=146)
    gfx.drawTextCentered(64, 146, "> Outdoor Weather [R]", COLOR_TEXT_MUTED, 1);
}

// ------------------------------------------------------------------------------
// Page 2: Outdoor Weather (OpenWeatherMap Sync)
// ------------------------------------------------------------------------------
void ClimateApp::renderOutdoorPage(RenderEngine& gfx, const TelemetryData& telem) {
    gfx.drawHeader("OUTDOOR (2/3)", 2);

    // 1. City & Condition Banner (y=39..67, h=28)
    gfx.drawCard(6, 39, 116, 28, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(10, 43, telem.weatherCity, COLOR_NEON_AMBER, 1);

    if (telem.weatherValid) {
        gfx.drawTextRight(118, 43, "LIVE SYNC", COLOR_NEON_MINT, 1);
    } else if (telem.wifiConnected) {
        gfx.drawTextRight(118, 43, "UPDATING", COLOR_NEON_CYAN, 1);
    } else {
        gfx.drawTextRight(118, 43, "NO WIFI", COLOR_NEON_RED, 1);
    }

    char descBuf[22];
    snprintf(descBuf, sizeof(descBuf), "%s", telem.weatherDesc);
    descBuf[0] = toupper(descBuf[0]);
    gfx.drawTextCentered(64, 54, descBuf, COLOR_TEXT_LIGHT, 1);

    // 2. Temperature & Feels Like Card (y=70..110, h=40)
    gfx.drawCard(6, 70, 116, 40, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(10, 74, "OUT TEMP", COLOR_TEXT_MUTED, 1);

    char outTBuf[12];
    snprintf(outTBuf, sizeof(outTBuf), "%.1f", telem.outdoorTemp);
    gfx.drawText(10, 85, outTBuf, COLOR_NEON_CYAN, 2);
    gfx.drawText(46, 74, "C", COLOR_TEXT_LIGHT, 1);
    gfx.drawPixel(42, 74, COLOR_NEON_CYAN);

    gfx.drawTextRight(118, 74, "FEELS LIKE", COLOR_TEXT_MUTED, 1);
    char feelsBuf[16];
    snprintf(feelsBuf, sizeof(feelsBuf), "%.1f C", telem.outdoorFeelsLike);
    gfx.drawTextRight(118, 86, feelsBuf, COLOR_NEON_AMBER, 1);

    char pressBuf[16];
    snprintf(pressBuf, sizeof(pressBuf), "%u hPa", telem.pressureHpa);
    gfx.drawTextRight(118, 98, pressBuf, COLOR_TEXT_MUTED, 1);

    // 3. Environmental Details: Humidity & Wind (y=113..143, h=30)
    gfx.drawCard(6, 113, 116, 30, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(10, 117, "HUMIDITY:", COLOR_TEXT_MUTED, 1);
    char outHBuf[12];
    snprintf(outHBuf, sizeof(outHBuf), "%.0f%%", telem.outdoorHumidity);
    gfx.drawText(62, 117, outHBuf, COLOR_NEON_MINT, 1);

    gfx.drawText(10, 129, "WIND:", COLOR_TEXT_MUTED, 1);
    char windBuf[16];
    snprintf(windBuf, sizeof(windBuf), "%.1fm/s", telem.windSpeedMps);
    gfx.drawText(40, 129, windBuf, COLOR_TEXT_LIGHT, 1);
    gfx.drawTextRight(118, 129, "OPENWEATHER", COLOR_TEXT_DARK, 1);

    // 4. Navigation Hint (y=146)
    gfx.drawTextCentered(64, 146, "[L] Indoor   [R] Compare", COLOR_TEXT_MUTED, 1);
}

// ------------------------------------------------------------------------------
// Page 3: Indoor vs Outdoor Delta Comparison & Smart Advisory
// ------------------------------------------------------------------------------
void ClimateApp::renderComparePage(RenderEngine& gfx, const TelemetryData& telem) {
    gfx.drawHeader("ADVISORY (3/3)", 2);

    float deltaT = telem.temperature - telem.outdoorTemp;
    float deltaH = telem.humidity - telem.outdoorHumidity;

    // 1. Temperature Delta Card (y=39..71, h=32)
    gfx.drawCard(6, 39, 116, 32, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(10, 43, "TEMP DELTA", COLOR_TEXT_MUTED, 1);

    char compTQuick[24];
    snprintf(compTQuick, sizeof(compTQuick), "IN:%.0fC | OUT:%.0fC", telem.temperature, telem.outdoorTemp);
    gfx.drawTextRight(118, 43, compTQuick, COLOR_TEXT_MUTED, 1);

    char deltaTMsg[28];
    if (fabs(deltaT) < 0.5f) {
        snprintf(deltaTMsg, sizeof(deltaTMsg), "Equal Indoors/Out");
    } else if (deltaT > 0) {
        snprintf(deltaTMsg, sizeof(deltaTMsg), "+%.1fC (Warmer Inside)", deltaT);
    } else {
        snprintf(deltaTMsg, sizeof(deltaTMsg), "%.1fC (Cooler Inside)", deltaT);
    }
    uint16_t tDeltaCol = (deltaT > 0) ? COLOR_NEON_AMBER : COLOR_NEON_CYAN;
    gfx.drawTextCentered(64, 56, deltaTMsg, tDeltaCol, 1);

    // 2. Humidity Delta Card (y=74..106, h=32)
    gfx.drawCard(6, 74, 116, 32, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(10, 78, "HUMIDITY DELTA", COLOR_TEXT_MUTED, 1);

    char compHQuick[24];
    snprintf(compHQuick, sizeof(compHQuick), "IN:%.0f%% | OUT:%.0f%%", telem.humidity, telem.outdoorHumidity);
    gfx.drawTextRight(118, 78, compHQuick, COLOR_TEXT_MUTED, 1);

    char deltaHMsg[28];
    if (fabs(deltaH) < 3.0f) {
        snprintf(deltaHMsg, sizeof(deltaHMsg), "Balanced Moisture");
    } else if (deltaH > 0) {
        snprintf(deltaHMsg, sizeof(deltaHMsg), "+%.0f%% (Humid Inside)", deltaH);
    } else {
        snprintf(deltaHMsg, sizeof(deltaHMsg), "%.0f%% (Drier Inside)", deltaH);
    }
    gfx.drawTextCentered(64, 91, deltaHMsg, COLOR_NEON_MINT, 1);

    // 3. Smart Climate Advisory Card (y=109..143, h=34)
    gfx.drawCard(6, 109, 116, 34, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawText(10, 113, "* AI CLIMATE ADVICE", COLOR_NEON_MINT, 1);

    const char* adv1 = "Optimal microclimate.";
    const char* adv2 = "Clean, balanced air.";

    if (telem.outdoorTemp < telem.temperature - 1.5f && telem.temperature > 24.5f) {
        adv1 = "Breeze outdoors: Open";
        adv2 = "windows to cool room.";
    } else if (telem.outdoorHumidity > 75.0f) {
        adv1 = "Humid outdoors: Keep";
        adv2 = "windows sealed.";
    } else if (telem.outdoorTemp > telem.temperature + 2.0f && telem.temperature < 20.0f) {
        adv1 = "Warmer outdoors: Open";
        adv2 = "blinds for sun heat.";
    } else if (telem.temperature < 18.5f) {
        adv1 = "Cool indoors: Maintain";
        adv2 = "heating comfort.";
    } else if (telem.humidity < 30.0f) {
        adv1 = "Dry indoor air: Use";
        adv2 = "humidifier or plants.";
    }

    gfx.drawText(10, 124, adv1, COLOR_TEXT_LIGHT, 1);
    gfx.drawText(10, 134, adv2, COLOR_TEXT_LIGHT, 1);

    // 4. Navigation Hint (y=146)
    gfx.drawTextCentered(64, 146, "[L] Weather   OK: Refresh", COLOR_TEXT_MUTED, 1);
}

bool ClimateApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK || evt.type == BUTTON_REPEAT) {
        if (evt.button == BTN_ID_RIGHT || evt.button == BTN_ID_DOWN) {
            _currentPage = (_currentPage + 1) % 3;
            return true;
        }
        if (evt.button == BTN_ID_LEFT || evt.button == BTN_ID_UP) {
            _currentPage = (_currentPage == 0) ? 2 : (_currentPage - 1);
            return true;
        }
        if (evt.button == BTN_ID_ENTER || evt.button == BTN_ID_SELECT) {
            RenderEngine::getInstance().showToast("WEATHER SYNC", "Fetching live data...", COLOR_NEON_AMBER, 1200);
            fetchLiveWeather();
            return true;
        }
    }
    return false;
}
