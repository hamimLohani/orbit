#include "src/network/wifi_manager.h"
#include "src/core/settings_manager.h"
#include "src/core/shared_data.h"
#include "src/core/event_bus.h"
#include "src/display/render_engine.h"
#include "src/display/theme.h"
#include <algorithm>

void WifiManager::init() {
    startStation();
}

void WifiManager::startStation() {
    String ssid = SettingsManager::getInstance().getWifiSsid();
    String pass = SettingsManager::getInstance().getWifiPass();
    bool offlineMode = SettingsManager::getInstance().getOfflineMode();

    WiFi.setSleep(false); // Low latency for WebSocket & HTTP

    bool isDummySsid = (ssid.length() == 0 || ssid == "ORBIT_WiFi" || ssid == "YOUR_WIFI_SSID" || ssid == "YOUR_SSID");
    bool isDummyPass = (pass == "SuperSecretPassword" || pass == "12345678");

    if (!offlineMode && !isDummySsid && !isDummyPass) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pass.c_str());
        _connecting = true;
        _connectStartTime = millis();
        _apActive = false;
        Serial.printf("[WiFi] Attempting connection to network: %s\n", ssid.c_str());
    } else {
        if (offlineMode) {
            Serial.println("[WiFi] Offline mode active — skipping station connection, starting setup hotspot.");
        } else {
            Serial.println("[WiFi] No custom WiFi configured or placeholder password, start AP mode for initial setup.");
        }
        startApFallback();
    }
}

void WifiManager::startApFallback() {
    // Stop any pending station association loops so the radio is fully available for SoftAP and scanning
    WiFi.disconnect(false);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(DEFAULT_AP_SSID, DEFAULT_AP_PASS);
    _apActive = true;
    _connecting = false;
    _apGracePeriodEnd = 0;

    // Start Captive Portal DNS server (redirect all domain queries on port 53 to softAP IP)
    _dnsServer.stop();
    _dnsServer.start(53, "*", WiFi.softAPIP());

    // Start mDNS responder
    MDNS.begin(MDNS_HOSTNAME);
    MDNS.addService("http", "tcp", HTTP_PORT);

    SharedData::getInstance().setNetwork(false, -45, WiFi.softAPIP().toString().c_str());
    Serial.printf("[WiFi] SoftAP started: %s (Password: %s) IP: %s\n", 
                  DEFAULT_AP_SSID, DEFAULT_AP_PASS, WiFi.softAPIP().toString().c_str());

    // Proactively scan 2.4 GHz networks so they are immediately cached when user opens setup portal
    startScan();
}

void WifiManager::connect(const String& ssid, const String& pass) {
    SettingsManager::getInstance().setWifiSsid(ssid);
    SettingsManager::getInstance().setWifiPass(pass);
    SettingsManager::getInstance().setOfflineMode(false); // Reset offline mode when connecting

    Serial.printf("[WiFi] User connecting to: %s\n", ssid.c_str());

    // Maintain AP_STA mode so the client device on SoftAP is not abruptly disconnected
    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect(false);
    WiFi.begin(ssid.c_str(), pass.c_str());
    _connecting = true;
    _connectStartTime = millis();
}

void WifiManager::disconnect() {
    WiFi.disconnect(true);
    _connecting = false;
    startApFallback();
}

void WifiManager::forgetWifi() {
    Serial.println("[WiFi] User requested to forget Wi-Fi credentials.");
    // 1. Erase from NVS flash storage and set offline mode
    SettingsManager::getInstance().setWifiSsid("");
    SettingsManager::getInstance().setWifiPass("");
    SettingsManager::getInstance().setOfflineMode(true); // Pattern from Smart-Multi-Plug: mark offline

    // 2. Erase from ESP32 WiFi STA stack
    WiFi.disconnect(true, true /* eraseAP = true */);
    _connecting = false;

    // 3. Immediately switch to SoftAP setup hotspot
    startApFallback();

    // 4. Update SharedData & EventBus
    SharedData::getInstance().setNetwork(false, -45, WiFi.softAPIP().toString().c_str());
    EventBus::getInstance().publish(EVENT_WIFI_DISCONNECTED, nullptr);

    // 5. Visual confirmation on physical screen
    RenderEngine::getInstance().showToast("WI-FI FORGOTTEN", "Hotspot: ORBIT-Setup", COLOR_NEON_AMBER, 3500);
}

bool WifiManager::isConnected() const {
    return (WiFi.status() == WL_CONNECTED);
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

String WifiManager::getSoftApIp() const {
    if (_apActive) {
        return WiFi.softAPIP().toString();
    }
    return "0.0.0.0";
}

String WifiManager::getSsid() const {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.SSID();
    }
    return SettingsManager::getInstance().getWifiSsid();
}

void WifiManager::startScan() {
    _scanRequested = true;
}

void WifiManager::handleBackgroundScan() {
    uint32_t now = millis();

    // 1. If currently scanning, check if the driver completed
    if (_scanning) {
        int16_t n = WiFi.scanComplete();
        if (n >= 0) {
            _scannedNetworks.clear();
            for (int i = 0; i < n; ++i) {
                String s = WiFi.SSID(i);
                s.trim();
                if (s.length() == 0) continue;

                // Deduplicate SSIDs, keeping strongest RSSI
                bool exists = false;
                for (auto& item : _scannedNetworks) {
                    if (item.ssid == s) {
                        exists = true;
                        if (WiFi.RSSI(i) > item.rssi) {
                            item.rssi = WiFi.RSSI(i);
                        }
                        break;
                    }
                }
                if (!exists) {
                    ScannedNetwork net;
                    net.ssid = s;
                    net.rssi = WiFi.RSSI(i);
                    net.secure = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                    net.channel = WiFi.channel(i);
                    _scannedNetworks.push_back(net);
                }
            }

            // Sort descending by signal strength
            std::sort(_scannedNetworks.begin(), _scannedNetworks.end(), [](const ScannedNetwork& a, const ScannedNetwork& b) {
                return a.rssi > b.rssi;
            });

            WiFi.scanDelete();
            _scanning = false;
            _scanRequested = false;
            _lastScanTime = now;
            Serial.printf("[WiFi] Scan complete. Found %d networks.\n", (int)_scannedNetworks.size());
        } else if (n == WIFI_SCAN_FAILED) {
            // Check timeout (5 seconds timeout)
            if (now - _scanStartTime > 5000) {
                _scanning = false;
                _scanRequested = false;
                _lastScanTime = now;
                Serial.println("[WiFi] Scan timed out or interrupted.");
            }
        }
        return;
    }

    // 2. If not scanning, check if a scan should be initiated
    // Trigger if:
    // - User requested a scan (_scanRequested)
    // - OR cache is empty and at least 3 seconds have passed since boot/last attempt
    // - OR 45 seconds have elapsed to keep the network list fresh
    bool shouldScan = _scanRequested || (_scannedNetworks.empty() && (now - _lastScanTime > 3000)) || (now - _lastScanTime > 45000);
    if (!shouldScan) return;

    _scanRequested = false;
    _lastScanTime = now;

    // Ensure Wi-Fi radio is in WIFI_AP_STA mode so scanning works alongside SoftAP
    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
    }
    WiFi.enableSTA(true);
    WiFi.scanDelete();

    // Use active scan with 150ms channel dwell time (fast, doesn't drop AP beacons)
    int16_t res = WiFi.scanNetworks(true /* async */, false /* show hidden */, false /* passive */, 150 /* max_ms_per_chan */);
    if (res == WIFI_SCAN_RUNNING) {
        _scanning = true;
        _scanStartTime = now;
        Serial.println("[WiFi] Background 2.4 GHz network scan initiated (150ms/ch)...");
    } else {
        Serial.printf("[WiFi] scanNetworks start returned: %d\n", res);
    }
}

String WifiManager::getScannedNetworksJson() {
    JsonDocument doc;
    doc["scanning"] = _scanning;
    doc["count"] = _scannedNetworks.size();
    JsonArray arr = doc["networks"].to<JsonArray>();
    for (const auto& net : _scannedNetworks) {
        JsonObject obj = arr.add<JsonObject>();
        obj["ssid"] = net.ssid;
        obj["rssi"] = net.rssi;
        obj["secure"] = net.secure;
        obj["channel"] = net.channel;
    }
    String out;
    serializeJson(doc, out);
    return out;
}

void WifiManager::update() {
    // Process captive portal DNS queries
    if (_apActive) {
        _dnsServer.processNextRequest();
    }

    // Process background Wi-Fi scan results (in SoftAP setup mode)
    if (_apActive) {
        handleBackgroundScan();
    }

    uint32_t now = millis();
    if (now - _lastCheckTime < 1000) return;
    _lastCheckTime = now;

    if (_connecting) {
        if (WiFi.status() == WL_CONNECTED) {
            _connecting = false;
            // Connected! Set grace period for SoftAP (30 seconds) so user gets feedback on phone
            _apGracePeriodEnd = now + 30000;

            MDNS.begin(MDNS_HOSTNAME);
            MDNS.addService("http", "tcp", HTTP_PORT);
            SharedData::getInstance().setNetwork(true, WiFi.RSSI(), WiFi.localIP().toString().c_str());
            EventBus::getInstance().publish(EVENT_WIFI_CONNECTED, nullptr);
            Serial.printf("[WiFi] Connected! IP: %s RSSI: %d dBm\n", WiFi.localIP().toString().c_str(), (int)WiFi.RSSI());
        } else if (now - _connectStartTime > 8000) { // 8s connection timeout (matches Smart-Multi-Plug)
            _connecting = false;
            WiFi.disconnect(false); // Abort pending association attempts so radio is fully idle
            Serial.println("[WiFi] Connection timed out after 8s. Switched station radio to idle.");
            // Ensure AP is active so user can re-try
            if (!_apActive) {
                startApFallback();
            }
        }
    } else if (WiFi.status() == WL_CONNECTED) {
        SharedData::getInstance().setNetwork(true, WiFi.RSSI(), WiFi.localIP().toString().c_str());
        // Once connected, shut down AP after grace period to conserve radio & power
        if (_apActive && _apGracePeriodEnd > 0 && now > _apGracePeriodEnd) {
            _dnsServer.stop();
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_STA);
            _apActive = false;
            _apGracePeriodEnd = 0;
            Serial.println("[WiFi] SoftAP grace period expired. Switched to Station-only mode.");
        }
    } else {
        // If not connected and not connecting, ensure AP is available for recovery
        if (!_apActive && !_connecting) {
            startApFallback();
        }
    }
}
