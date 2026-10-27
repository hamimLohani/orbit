#pragma once
#include <Arduino.h>
#include <Preferences.h>

// ==============================================================================
// ORBIT v2 — Persistent Settings Manager (ESP32 NVS)
// ==============================================================================

class SettingsManager {
public:
    static SettingsManager& getInstance() {
        static SettingsManager instance;
        return instance;
    }

    void init();

    // Core Settings Getters & Setters
    String getWifiSsid();
    void setWifiSsid(const String& ssid);

    String getWifiPass();
    void setWifiPass(const String& pass);

    uint8_t getBrightness();
    void setBrightness(uint8_t brightness);

    uint8_t getThemeId();
    void setThemeId(uint8_t id);

    uint8_t getRgbMode();
    void setRgbMode(uint8_t mode);

    uint32_t getRgbColor();
    void setRgbColor(uint32_t color);

    float getNoiseThreshold();
    void setNoiseThreshold(float threshold);

    uint16_t getPomodoroFocusMins();
    void setPomodoroFocusMins(uint16_t mins);

    uint16_t getPomodoroBreakMins();
    void setPomodoroBreakMins(uint16_t mins);

    // Generic Key-Value API for Future Apps
    int getAppInt(const char* appId, const char* key, int defaultVal = 0);
    void setAppInt(const char* appId, const char* key, int value);

    String getAppString(const char* appId, const char* key, const String& defaultVal = "");
    void setAppString(const char* appId, const char* key, const String& value);

    bool getAppBool(const char* appId, const char* key, bool defaultVal = false);
    void setAppBool(const char* appId, const char* key, bool value);

    void factoryReset();

private:
    SettingsManager() = default;
    ~SettingsManager() = default;
    Preferences _prefs;
};
