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

    WiFi.setSleep(false); // Low latency for WebSocket & HTTP

    bool isDummySsid = (ssid.length() == 0 || ssid == "ORBIT_WiFi" || ssid == "YOUR_WIFI_SSID" || ssid == "YOUR_SSID");
    bool isDummyPass = (pass == "SuperSecretPassword" || pass == "12345678");

    if (!isDummySsid && !isDummyPass) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pass.c_str());
        _connecting = true;
        _connectStartTime = millis();
        _apActive = false;
        Serial.printf("[WiFi] Attempting connection to network: %s\n", ssid.c_str());
    } else {
        // No custom WiFi configured or placeholder password, start AP mode for initial setup
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
    // 1. Erase from NVS flash storage
    SettingsManager::getInstance().setWifiSsid("");
    SettingsManager::getInstance().setWifiPass("");

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
    if (_scanning) return;

    // Disconnect pending STA connection so ESP-IDF doesn't reject scan with ESP_ERR_WIFI_STATE
    if (_connecting || WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect(false);
        _connecting = false;
        delay(50);
    }

    WiFi.mode(WIFI_AP_STA);
    WiFi.enableSTA(true);
    WiFi.scanDelete();

    // Standard 2-param async scan (300ms default active dwell time)
    int16_t res = WiFi.scanNetworks(true /* async */, false /* show hidden */);
    if (res == WIFI_SCAN_RUNNING) {
        _scanning = true;
        _scanStartTime = millis();
        Serial.println("[WiFi] Async 2.4 GHz network scan initiated...");
        return;
    }

    Serial.printf("[WiFi] Async scan start returned: %d. Falling back to synchronous scan...\n", res);
    // If async scan failed (e.g. transient radio state transition), execute synchronous scan fallback
    int16_t n = WiFi.scanNetworks(false /* async */, false /* show hidden */);
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
        Serial.printf("[WiFi] Synchronous scan complete. Found %d networks.\n", (int)_scannedNetworks.size());
    } else {
        _scanning = false;
        Serial.printf("[WiFi] Synchronous scan also returned: %d\n", n);
    }
}

void WifiManager::checkScanCompletion() {
    if (!_scanning) return;
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
        Serial.printf("[WiFi] Scan complete. Found %d networks.\n", (int)_scannedNetworks.size());
    } else if (n == WIFI_SCAN_FAILED) {
        // Only mark failed if scan has been running for at least 1500ms to avoid premature failure
        if (millis() - _scanStartTime > 1500) {
            _scanning = false;
            Serial.println("[WiFi] Scan failed or interrupted.");
        }
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

    // Process background Wi-Fi scan results
    checkScanCompletion();

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
        } else if (now - _connectStartTime > 16000) { // 16s connection timeout
            _connecting = false;
            WiFi.disconnect(false); // Abort pending association attempts so radio is fully idle
            Serial.println("[WiFi] Connection timed out. Switched station radio to idle.");
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
