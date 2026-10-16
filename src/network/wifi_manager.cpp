#include "src/network/wifi_manager.h"
#include "src/core/settings_manager.h"
#include "src/core/shared_data.h"
#include "src/core/event_bus.h"

void WifiManager::init() {
    startStation();
}

void WifiManager::startStation() {
    String ssid = SettingsManager::getInstance().getWifiSsid();
    String pass = SettingsManager::getInstance().getWifiPass();

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false); // Low latency for WebSocket

    if (ssid.length() > 0 && ssid != DEFAULT_WIFI_SSID) {
        WiFi.begin(ssid.c_str(), pass.c_str());
        _connecting = true;
        _connectStartTime = millis();
    } else {
        // No custom WiFi configured, start AP mode for easy setup
        startApFallback();
    }
}

void WifiManager::startApFallback() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(DEFAULT_AP_SSID, DEFAULT_AP_PASS);
    _apActive = true;
    _connecting = false;

    // Start mDNS
    MDNS.begin(MDNS_HOSTNAME);
    MDNS.addService("http", "tcp", HTTP_PORT);

    SharedData::getInstance().setNetwork(true, -40, WiFi.softAPIP().toString().c_str());
}

void WifiManager::connect(const String& ssid, const String& pass) {
    SettingsManager::getInstance().setWifiSsid(ssid);
    SettingsManager::getInstance().setWifiPass(pass);
    WiFi.disconnect(true);
    WiFi.begin(ssid.c_str(), pass.c_str());
    _connecting = true;
    _connectStartTime = millis();
}

bool WifiManager::isConnected() const {
    return (WiFi.status() == WL_CONNECTED) || _apActive;
}

int8_t WifiManager::getRssi() const {
    return WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : -50;
}

String WifiManager::getIpAddress() const {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    } else if (_apActive) {
        return WiFi.softAPIP().toString();
    }
    return "0.0.0.0";
}

void WifiManager::update() {
    uint32_t now = millis();
    if (now - _lastCheckTime < 1000) return;
    _lastCheckTime = now;

    if (_connecting) {
        if (WiFi.status() == WL_CONNECTED) {
            _connecting = false;
            _apActive = false;
            MDNS.begin(MDNS_HOSTNAME);
            MDNS.addService("http", "tcp", HTTP_PORT);
            SharedData::getInstance().setNetwork(true, WiFi.RSSI(), WiFi.localIP().toString().c_str());
            EventBus::getInstance().publish(EVENT_WIFI_CONNECTED, nullptr);
        } else if (now - _connectStartTime > 15000) { // 15s timeout
            _connecting = false;
            startApFallback();
        }
    } else if (WiFi.status() == WL_CONNECTED) {
        SharedData::getInstance().setNetwork(true, WiFi.RSSI(), WiFi.localIP().toString().c_str());
    }
}
