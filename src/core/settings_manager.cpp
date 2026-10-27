#include "src/core/settings_manager.h"
#include "include/config.h"

void SettingsManager::init() {
    _prefs.begin("orbit", false);
}

String SettingsManager::getWifiSsid() {
    return _prefs.getString("wifi_ssid", DEFAULT_WIFI_SSID);
}

void SettingsManager::setWifiSsid(const String& ssid) {
    _prefs.putString("wifi_ssid", ssid);
}

String SettingsManager::getWifiPass() {
    return _prefs.getString("wifi_pass", DEFAULT_WIFI_PASS);
}

void SettingsManager::setWifiPass(const String& pass) {
    _prefs.putString("wifi_pass", pass);
}

uint8_t SettingsManager::getBrightness() {
    return _prefs.getUChar("bright", 80);
}

void SettingsManager::setBrightness(uint8_t brightness) {
    _prefs.putUChar("bright", brightness);
}

uint8_t SettingsManager::getThemeId() {
    return _prefs.getUChar("theme", 0);
}

void SettingsManager::setThemeId(uint8_t id) {
    _prefs.putUChar("theme", id);
}

uint8_t SettingsManager::getRgbMode() {
    return _prefs.getUChar("rgb_mode", 0); // Default to RGB_OFF (0)
}

void SettingsManager::setRgbMode(uint8_t mode) {
    _prefs.putUChar("rgb_mode", mode);
}

uint32_t SettingsManager::getRgbColor() {
    return _prefs.getUInt("rgb_color", 0x00FFFF);
}

void SettingsManager::setRgbColor(uint32_t color) {
    _prefs.putUInt("rgb_color", color);
}

float SettingsManager::getNoiseThreshold() {
    return _prefs.getFloat("noise_th", 65.0f);
}

void SettingsManager::setNoiseThreshold(float threshold) {
    _prefs.putFloat("noise_th", threshold);
}

uint16_t SettingsManager::getPomodoroFocusMins() {
    return _prefs.getUShort("pomo_focus", 25);
}

void SettingsManager::setPomodoroFocusMins(uint16_t mins) {
    _prefs.putUShort("pomo_focus", mins);
}

int SettingsManager::getAppInt(const char* appId, const char* key, int defaultVal) {
    String prefKey = String(appId) + "_" + String(key);
    return _prefs.getInt(prefKey.c_str(), defaultVal);
}

void SettingsManager::setAppInt(const char* appId, const char* key, int value) {
    String prefKey = String(appId) + "_" + String(key);
    _prefs.putInt(prefKey.c_str(), value);
}

String SettingsManager::getAppString(const char* appId, const char* key, const String& defaultVal) {
    String prefKey = String(appId) + "_" + String(key);
    return _prefs.getString(prefKey.c_str(), defaultVal);
}

void SettingsManager::setAppString(const char* appId, const char* key, const String& value) {
    String prefKey = String(appId) + "_" + String(key);
    _prefs.putString(prefKey.c_str(), value);
}

bool SettingsManager::getAppBool(const char* appId, const char* key, bool defaultVal) {
    String prefKey = String(appId) + "_" + String(key);
    return _prefs.getBool(prefKey.c_str(), defaultVal);
}

void SettingsManager::setAppBool(const char* appId, const char* key, bool value) {
    String prefKey = String(appId) + "_" + String(key);
    _prefs.putBool(prefKey.c_str(), value);
}

void SettingsManager::factoryReset() {
    _prefs.clear();
}
