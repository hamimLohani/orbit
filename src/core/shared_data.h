#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "include/config.h"

// ==============================================================================
// ORBIT v2 — Central Thread-Safe Telemetry Bus
// ==============================================================================

struct TelemetryData {
    // Environmental
    float temperature = 22.0f;
    float humidity = 50.0f;
    float heatIndex = 22.0f;

    // Audio & Noise
    float micRms = 0.0f;
    float micPeak = 0.0f;
    float micDb = 35.0f;
    uint8_t fftBands[FFT_NUM_BINS] = {0};
    bool noiseDistraction = false;

    // Time & Astral
    char timeStr[12] = "12:00:00";
    char dateStr[24] = "Jan 01, 2026";
    bool timeSynced = false;
    float moonPhase = 0.5f;     // 0.0 = New, 0.5 = Full
    bool isNightTime = false;

    // Network & System
    bool wifiConnected = false;
    int8_t wifiRssi = -100;
    char ipAddress[16] = "0.0.0.0";
    uint32_t freeHeap = 0;
    uint32_t uptimeSec = 0;
    char activeAppId[16] = "home";

    // Lighting State
    uint8_t rgbMode = 0;        // Off
    uint32_t rgbColor = 0x000000;
    uint8_t brightness = 80;    // 0-100%

    // Outdoor Live Weather (OpenWeatherMap Sync)
    bool weatherValid = false;
    float outdoorTemp = 21.5f;
    float outdoorFeelsLike = 21.0f;
    float outdoorHumidity = 58.0f;
    float windSpeedMps = 3.6f;
    uint16_t pressureHpa = 1014;
    char weatherCondition[20] = "Clear";
    char weatherCity[20] = "London";
    char weatherDesc[32] = "Clear Sky";
};

class SharedData {
public:
    static SharedData& getInstance() {
        static SharedData instance;
        return instance;
    }

    void init();

    // Thread-safe copy get
    TelemetryData getData();

    // Safe field setters
    void setClimate(float temp, float hum);
    void setWeather(float temp, float feelsLike, float hum, float windSpeed, uint16_t pressure, const char* condition, const char* city, const char* desc = nullptr);
    void setAudio(float rms, float peak, float db, const uint8_t* bins);
    void setNoiseDistraction(bool alert);
    void setTime(const char* time, const char* date, bool isNight, bool synced = true);
    void setMoonPhase(float phase);
    void setNetwork(bool connected, int8_t rssi, const char* ip);
    void setSystemStatus(uint32_t heap, uint32_t uptime, const char* appId);
    void setLighting(uint8_t mode, uint32_t color, uint8_t brightness);

private:
    SharedData();
    ~SharedData();
    SharedData(const SharedData&) = delete;
    SharedData& operator=(const SharedData&) = delete;

    TelemetryData _data;
    SemaphoreHandle_t _mutex;
};
