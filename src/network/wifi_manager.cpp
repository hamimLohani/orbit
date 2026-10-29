#include "src/network/wifi_manager.h"
#include "src/core/settings_manager.h"
#include "src/core/shared_data.h"
#include "src/core/event_bus.h"
#include "src/display/render_engine.h"
#include "src/display/theme.h"
#include <esp_wifi.h>
#include <algorithm>

void WifiManager::init() {
    startStation();
}

void WifiManager::startStation() {
    String ssid = SettingsManager::getInstance().getWifiSsid();
    String pass = SettingsManager::getInstance().getWifiPass();
    bool offlineMode = SettingsManager::getInstance().getOfflineMode();

    WiFi.setSleep(false); // Low latency for WebSocket & HTTP
    WiFi.setAutoReconnect(false); // Disable auto-reconnect to prevent ESP-IDF background connection locks

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
    _connecting = false;
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(false);
    delay(40);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(DEFAULT_AP_SSID, DEFAULT_AP_PASS);
    _apActive = true;
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

    // Proactively scan networks so they are immediately cached when user opens setup portal
    startScan();
}

void WifiManager::connect(const String& ssid, const String& pass) {
    SettingsManager::getInstance().setWifiSsid(ssid);
    SettingsManager::getInstance().setWifiPass(pass);
    SettingsManager::getInstance().setOfflineMode(false); // Reset offline mode when connecting

    Serial.printf("[WiFi] User connecting to: %s\n", ssid.c_str());

    // Maintain AP_STA mode so the client device on SoftAP is not abruptly disconnected
    WiFi.setAutoReconnect(false);
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

void WifiManager::performScan() {
    if (_scanning || _connecting) return;
    _scanning = true;
    _scanStartTime = millis();

    Serial.println("[WiFi] Starting native ESP-IDF Wi-Fi scan in taskNetwork...");

    // 1. Ensure radio is in clean state: no auto-reconnect loops
    WiFi.setAutoReconnect(false);
    esp_wifi_disconnect();
    delay(40);

    // 2. Ensure dual AP+STA mode
    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
        delay(40);
    }

    // 3. Configure scan with worldwide channels (1-14) and home channel dwell time so SoftAP clients do NOT drop
    wifi_country_t country = {
        .cc = "01",
        .schan = 1,
        .nchan = 14,
        .max_tx_power = 20,
        .policy = WIFI_COUNTRY_POLICY_MANUAL
    };
    esp_wifi_set_country(&country);

    wifi_scan_config_t config;
    memset(&config, 0, sizeof(wifi_scan_config_t));
    config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    config.scan_time.active.min = 20;
    config.scan_time.active.max = 120; // 120ms per channel
    config.home_chan_dwell_time = 30;  // 30ms back on AP channel 1 between channels
    config.show_hidden = true;

    // 4. Synchronous blocking scan inside taskNetwork (Core 0 priority 1)
    esp_err_t err = esp_wifi_scan_start(&config, true /* block */);
    Serial.printf("[WiFi] esp_wifi_scan_start: 0x%x (%s)\n", err, esp_err_to_name(err));

    std::vector<ScannedNetwork> newNetworks;

    if (err == ESP_OK) {
        uint16_t apCount = 0;
        esp_wifi_scan_get_ap_num(&apCount);
        Serial.printf("[WiFi] Driver found %u AP records.\n", apCount);

        if (apCount > 0) {
            wifi_ap_record_t* apList = (wifi_ap_record_t*)malloc(apCount * sizeof(wifi_ap_record_t));
            if (apList) {
                if (esp_wifi_scan_get_ap_records(&apCount, apList) == ESP_OK) {
                    for (uint16_t i = 0; i < apCount; ++i) {
                        String s = (const char*)apList[i].ssid;
                        s.trim();
                        if (s.length() == 0) continue;

                        bool exists = false;
                        for (auto& item : newNetworks) {
                            if (item.ssid == s) {
                                exists = true;
                                if (apList[i].rssi > item.rssi) item.rssi = apList[i].rssi;
                                break;
                            }
                        }
                        if (!exists) {
                            ScannedNetwork net;
                            net.ssid = s;
                            net.rssi = apList[i].rssi;
                            net.secure = (apList[i].authmode != WIFI_AUTH_OPEN);
                            net.channel = apList[i].primary;
                            newNetworks.push_back(net);
                        }
                    }
                }
                free(apList);
            }
        }
    }

    // 5. Fallback to standard WiFi.scanNetworks if native scan returned empty or transient error
    if (newNetworks.empty()) {
        int16_t n = WiFi.scanNetworks(false, false);
        Serial.printf("[WiFi] Fallback WiFi.scanNetworks returned: %d\n", n);
        if (n > 0) {
            for (int i = 0; i < n; ++i) {
                String s = WiFi.SSID(i);
                s.trim();
                if (s.length() == 0) continue;
                bool exists = false;
                for (auto& item : newNetworks) {
                    if (item.ssid == s) {
                        exists = true;
                        if (WiFi.RSSI(i) > item.rssi) item.rssi = WiFi.RSSI(i);
                        break;
                    }
                }
                if (!exists) {
                    ScannedNetwork net;
                    net.ssid = s;
                    net.rssi = WiFi.RSSI(i);
                    net.secure = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                    net.channel = WiFi.channel(i);
                    newNetworks.push_back(net);
                }
            }
            WiFi.scanDelete();
        }
    }

    if (!newNetworks.empty()) {
        std::sort(newNetworks.begin(), newNetworks.end(), [](const ScannedNetwork& a, const ScannedNetwork& b) {
            return a.rssi > b.rssi;
        });
        _scannedNetworks = std::move(newNetworks);
        Serial.printf("[WiFi] Cached %d unique networks.\n", (int)_scannedNetworks.size());
    } else {
        Serial.println("[WiFi] No networks found in current scan pass (retaining previous cache if any).");
    }

    _scanning = false;
    _scanRequested = false;
    _lastScanTime = millis();
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

    // Process background Wi-Fi scan trigger (in SoftAP setup mode)
    if (_apActive) {
        uint32_t now = millis();
        bool shouldScan = _scanRequested || (_scannedNetworks.empty() && (now - _lastScanTime > 3000)) || (now - _lastScanTime > 60000);
        if (shouldScan && !_scanning && !_connecting) {
            _scanRequested = false;
            performScan();
        }
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
            WiFi.setAutoReconnect(false);
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
