#include "src/network/wifi_manager.h"
#include "src/core/settings_manager.h"
#include "src/core/shared_data.h"
#include "src/core/event_bus.h"
#include "src/display/render_engine.h"
#include "src/display/theme.h"
#include <esp_wifi.h>
#include <algorithm>

void WifiManager::init() {
    // Monitor detailed STA connection events
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
            _connecting = false;
            _connectStatus = "connected";
            _authFailCount = 0;
            _noApCount = 0;

            // Save confirmed working credentials into NVS storage
            SettingsManager::getInstance().setWifiSsid(_pendingSsid);
            SettingsManager::getInstance().setWifiPass(_pendingPass);
            SettingsManager::getInstance().setOfflineMode(false);

            // Keep SoftAP alive for 60s so user's phone browser receives the success response
            _apGracePeriodEnd = millis() + 60000;

            MDNS.begin(MDNS_HOSTNAME);
            MDNS.addService("http", "tcp", HTTP_PORT);
            SharedData::getInstance().setNetwork(true, WiFi.RSSI(), WiFi.localIP().toString().c_str());
            EventBus::getInstance().publish(EVENT_WIFI_CONNECTED, nullptr);

            char toastMsg[32];
            snprintf(toastMsg, sizeof(toastMsg), "IP: %s", WiFi.localIP().toString().c_str());
            RenderEngine::getInstance().showToast("WI-FI CONNECTED", toastMsg, COLOR_NEON_MINT, 5000);
            Serial.printf("[WiFi] STA Got IP: %s\n", WiFi.localIP().toString().c_str());
        } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
            uint8_t reason = info.wifi_sta_disconnected.reason;
            Serial.printf("[WiFi] STA Disconnected (reason %u: %s)\n", 
                          reason, WiFi.STA.disconnectReasonName((wifi_err_reason_t)reason));
            if (_connecting) {
                if (reason == WIFI_REASON_AUTH_FAIL || 
                    reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT || 
                    reason == WIFI_REASON_HANDSHAKE_TIMEOUT ||
                    reason == WIFI_REASON_MIC_FAILURE) {
                    _authFailCount++;
                    if (_authFailCount >= 2) {
                        _connecting = false;
                        _connectStatus = "failed";
                        WiFi.disconnect(false);
                        RenderEngine::getInstance().showToast("AUTH FAILED", "Wrong password", COLOR_NEON_RED, 4000);
                        Serial.println("[WiFi] Authentication failed: incorrect password.");
                        if (!_apActive) startApFallback();
                    }
                } else if (reason == WIFI_REASON_NO_AP_FOUND) {
                    _noApCount++;
                    if (_noApCount >= 2) {
                        _connecting = false;
                        _connectStatus = "not_found";
                        WiFi.disconnect(false);
                        RenderEngine::getInstance().showToast("NOT FOUND", "SSID out of range", COLOR_NEON_RED, 4000);
                        Serial.println("[WiFi] Network not found.");
                        if (!_apActive) startApFallback();
                    }
                }
            }
        }
    });

    startStation();
}

void WifiManager::startStation() {
    String ssid = SettingsManager::getInstance().getWifiSsid();
    String pass = SettingsManager::getInstance().getWifiPass();
    bool offlineMode = SettingsManager::getInstance().getOfflineMode();

    WiFi.setSleep(false);
    WiFi.setAutoReconnect(false);

    bool isDummySsid = (ssid.length() == 0 || ssid == "ORBIT_WiFi" || ssid == "YOUR_WIFI_SSID" || ssid == "YOUR_SSID" || ssid == "YourWiFiSSID");
    bool isDummyPass = (pass == "SuperSecretPassword" || pass == "12345678");

    if (!offlineMode && !isDummySsid && !isDummyPass) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pass.length() > 0 ? pass.c_str() : nullptr);
        _connecting = true;
        _connectStatus = "connecting";
        _pendingSsid = ssid;
        _pendingPass = pass;
        _connectStartTime = millis();
        _apActive = false;
        Serial.printf("[WiFi] Attempting connection to network: %s\n", ssid.c_str());
    } else {
        if (offlineMode) {
            Serial.println("[WiFi] Offline mode active — skipping station connection, starting setup hotspot.");
        } else {
            Serial.println("[WiFi] No saved Wi-Fi credentials found. Starting ORBIT-Setup hotspot.");
        }
        startApFallback();
    }
}

void WifiManager::populateScannedNetworks(int16_t count) {
    if (count <= 0) return;

    std::vector<ScannedNetwork> newNetworks;
    for (int i = 0; i < count; ++i) {
        String s = WiFi.SSID(i);
        s.trim();
        if (s.length() == 0) continue;

        bool exists = false;
        for (auto& item : newNetworks) {
            if (item.ssid == s) {
                exists = true;
                if (WiFi.RSSI(i) > item.rssi) {
                    item.rssi = WiFi.RSSI(i);
                    item.secure = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                    item.channel = WiFi.channel(i);
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
            newNetworks.push_back(net);
        }
    }

    if (!newNetworks.empty()) {
        std::sort(newNetworks.begin(), newNetworks.end(), [](const ScannedNetwork& a, const ScannedNetwork& b) {
            return a.rssi > b.rssi;
        });
        _scannedNetworks = std::move(newNetworks);
        Serial.printf("[WiFi] Successfully cached %u unique wireless networks.\n", (unsigned)_scannedNetworks.size());
    }
}

int16_t WifiManager::scanSync() {
    Serial.println("[WiFi] Starting standard synchronous Wi-Fi scan (AgroSense pattern)...");
    WiFi.enableSTA(true);
    WiFi.scanDelete();

    // Standard synchronous scan with factory TX power and default parameters
    int16_t n = WiFi.scanNetworks(false /* blocking */, true /* show_hidden */);
    Serial.printf("[WiFi] WiFi.scanNetworks returned: %d networks.\n", n);

    if (n > 0) {
        populateScannedNetworks(n);
        char toastMsg[32];
        snprintf(toastMsg, sizeof(toastMsg), "%d Nets Found", n);
        RenderEngine::getInstance().showToast("WI-FI SCAN", toastMsg, COLOR_NEON_MINT, 3000);
    } else {
        Serial.println("[WiFi] Scan returned 0 networks.");
        RenderEngine::getInstance().showToast("WI-FI SCAN", "0 Nets Found", COLOR_NEON_AMBER, 3000);
    }
    WiFi.scanDelete();
    _lastScanTime = millis();
    return n;
}

void WifiManager::startApFallback() {
    _connecting = false;
    _connectStatus = "idle";
    WiFi.setAutoReconnect(false);

    // 1. If cache is empty, do a clean synchronous STA scan BEFORE launching SoftAP (AgroSense pattern)
    if (_scannedNetworks.empty()) {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect(false);
        delay(50);
        scanSync();
    }

    // 2. Now launch dual AP+STA mode and SoftAP
    WiFi.mode(WIFI_AP_STA);
    delay(40);

    const char* apPass = (strlen(DEFAULT_AP_PASS) >= 8) ? DEFAULT_AP_PASS : nullptr;
    WiFi.softAP(DEFAULT_AP_SSID, apPass);

    _apActive = true;
    _apGracePeriodEnd = 0;

    // Start Captive Portal DNS server (redirect all domain queries on port 53 to softAP IP)
    _dnsServer.stop();
    _dnsServer.start(53, "*", WiFi.softAPIP());

    // Start mDNS responder
    MDNS.begin(MDNS_HOSTNAME);
    MDNS.addService("http", "tcp", HTTP_PORT);

    SharedData::getInstance().setNetwork(false, -45, WiFi.softAPIP().toString().c_str());
    Serial.printf("[WiFi] SoftAP started: %s (Password: %s) IP: %s | Cached: %u networks\n", 
                  DEFAULT_AP_SSID, apPass ? apPass : "None (Open Network)", 
                  WiFi.softAPIP().toString().c_str(), (unsigned)_scannedNetworks.size());
}

void WifiManager::connect(const String& ssid, const String& pass) {
    _pendingSsid = ssid;
    _pendingPass = pass;
    _connectStatus = "connecting";
    _authFailCount = 0;
    _noApCount = 0;

    // Detect if we know the channel of the network
    _pendingChannel = 0;
    for (const auto& net : _scannedNetworks) {
        if (net.ssid == ssid && net.channel > 0) {
            _pendingChannel = net.channel;
            break;
        }
    }

    // Immediately persist credentials so they are safely preserved
    SettingsManager::getInstance().setWifiSsid(_pendingSsid);
    SettingsManager::getInstance().setWifiPass(_pendingPass);
    SettingsManager::getInstance().setOfflineMode(false);

    Serial.printf("[WiFi] Queued connection to: %s (Channel: %u)\n", ssid.c_str(), _pendingChannel);

    char toastMsg[32];
    snprintf(toastMsg, sizeof(toastMsg), "To: %s", ssid.c_str());
    RenderEngine::getInstance().showToast("CONNECTING WI-FI", toastMsg, COLOR_NEON_CYAN, 5000);

    // Schedule radio transition after 350ms so AsyncWebServer finishes flushing HTTP 200 to client
    _pendingConnect = true;
    _pendingConnectTime = millis() + 350;
}

void WifiManager::disconnect() {
    WiFi.disconnect(true);
    _connecting = false;
    _connectStatus = "idle";
    startApFallback();
}

void WifiManager::forgetWifi() {
    Serial.println("[WiFi] User requested to forget Wi-Fi credentials.");
    // 1. Erase from NVS flash storage and set offline mode
    SettingsManager::getInstance().setWifiSsid("");
    SettingsManager::getInstance().setWifiPass("");
    SettingsManager::getInstance().setOfflineMode(true);

    // 2. Erase from ESP32 WiFi STA stack
    WiFi.disconnect(true, true /* eraseAP = true */);
    _connecting = false;
    _connectStatus = "idle";
    _scannedNetworks.clear(); // Clear cache to trigger fresh scan on AP fallback

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
    if (_scanning || _connecting) return;

    int16_t status = WiFi.scanComplete();
    if (status == WIFI_SCAN_RUNNING) {
        _scanning = true;
        return;
    }

    WiFi.scanDelete();
    WiFi.enableSTA(true);

    _scanning = true;
    _scanStartTime = millis();

    int16_t res = WiFi.scanNetworks(true /* async */, true /* show_hidden */);
    Serial.printf("[WiFi] Background scanNetworks started: %d\n", res);

    if (res == WIFI_SCAN_FAILED) {
        _scanning = false;
        Serial.println("[WiFi] scanNetworks failed to initiate.");
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

String WifiManager::buildNetworkSelectHtml() {
    String select = "<select id=\"wifi-select\" name=\"ssid_select\">\n";
    select += "<option value=\"\" disabled selected>-- Select an available Wi-Fi network --</option>\n";

    if (_scannedNetworks.empty()) {
        select += "<option value=\"\" disabled>No networks found (click Rescan)</option>\n";
    } else {
        for (const auto& net : _scannedNetworks) {
            String lockStr = net.secure ? " [🔒]" : " [Open]";
            String bar;
            if      (net.rssi >= -55) bar = "▂▄▆█";
            else if (net.rssi >= -65) bar = "▂▄▆ ";
            else if (net.rssi >= -75) bar = "▂▄  ";
            else                      bar = "▂   ";

            String cleanSsid = net.ssid;
            cleanSsid.replace("&", "&amp;");
            cleanSsid.replace("\"", "&quot;");
            cleanSsid.replace("<", "&lt;");
            cleanSsid.replace(">", "&gt;");

            select += "<option value=\"" + cleanSsid + "\" data-secure=\"" + (net.secure ? "true" : "false") + "\">";
            select += bar + " " + cleanSsid + lockStr + " (" + String(net.rssi) + " dBm)";
            select += "</option>\n";
        }
    }
    select += "<option value=\"__custom__\">+ Enter network manually (Hidden SSID)...</option>\n";
    select += "</select>\n";
    return select;
}

String WifiManager::buildNetworkCardsHtml() {
    if (_scannedNetworks.empty()) {
        return "<div class=\"empty-state\">No networks discovered yet.<br>Click <strong>Rescan</strong> or enter SSID manually below.</div>";
    }
    String html = "";
    for (const auto& net : _scannedNetworks) {
        String lockSvg = net.secure ? 
            "<svg width=\"13\" height=\"13\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"var(--muted)\" stroke-width=\"2.2\"><rect x=\"3\" y=\"11\" width=\"18\" height=\"11\" rx=\"2\" ry=\"2\"></rect><path d=\"M7 11V7a5 5 0 0 1 10 0v4\"></path></svg>" : 
            "<span class=\"badge-open\">OPEN</span>";

        int bars = net.rssi > -60 ? 4 : (net.rssi > -70 ? 3 : (net.rssi > -80 ? 2 : 1));
        String barColor = net.rssi > -65 ? "var(--mint)" : (net.rssi > -78 ? "var(--cyan)" : "var(--amber)");
        String sigSvg = "<svg width=\"15\" height=\"15\" viewBox=\"0 0 24 24\" fill=\"" + barColor + "\">";
        sigSvg += "<rect x=\"2\" y=\"16\" width=\"3\" height=\"6\" rx=\"1\" opacity=\"" + String(bars >= 1 ? "1" : "0.25") + "\"/>";
        sigSvg += "<rect x=\"7\" y=\"12\" width=\"3\" height=\"10\" rx=\"1\" opacity=\"" + String(bars >= 2 ? "1" : "0.25") + "\"/>";
        sigSvg += "<rect x=\"12\" y=\"8\" width=\"3\" height=\"14\" rx=\"1\" opacity=\"" + String(bars >= 3 ? "1" : "0.25") + "\"/>";
        sigSvg += "<rect x=\"17\" y=\"4\" width=\"3\" height=\"18\" rx=\"1\" opacity=\"" + String(bars >= 4 ? "1" : "0.25") + "\"/>";
        sigSvg += "</svg>";

        String escapedSsid = net.ssid;
        escapedSsid.replace("\\", "\\\\");
        escapedSsid.replace("'", "\\'");
        escapedSsid.replace("\"", "&quot;");

        String displaySsid = net.ssid;
        displaySsid.replace("&", "&amp;");
        displaySsid.replace("<", "&lt;");
        displaySsid.replace(">", "&gt;");

        html += "<div class=\"net-item\" data-ssid=\"" + escapedSsid + "\" onclick=\"selectNetwork('" + escapedSsid + "', " + (net.secure ? "true" : "false") + ")\">";
        html += "<div class=\"net-name\">" + lockSvg + "<span>" + displaySsid + "</span></div>";
        html += "<div class=\"net-meta\"><span class=\"net-rssi\">" + String(net.rssi) + " dBm</span>" + sigSvg + "</div>";
        html += "</div>";
    }
    return html;
}

void WifiManager::update() {
    uint32_t now = millis();

    // 1. Drain Captive Portal DNS queries (process up to 5 per tick to handle bursts)
    if (_apActive) {
        for (int i = 0; i < 5; ++i) {
            _dnsServer.processNextRequest();
        }
    }

    // 1b. Handle scheduled Wi-Fi radio connection after HTTP 200 flush
    if (_pendingConnect && now >= _pendingConnectTime) {
        _pendingConnect = false;
        WiFi.setAutoReconnect(false);
        WiFi.mode(WIFI_AP_STA);
        WiFi.disconnect(false);
        if (_pendingChannel > 0) {
            WiFi.begin(_pendingSsid.c_str(), _pendingPass.length() > 0 ? _pendingPass.c_str() : nullptr, _pendingChannel);
        } else {
            WiFi.begin(_pendingSsid.c_str(), _pendingPass.length() > 0 ? _pendingPass.c_str() : nullptr);
        }
        _connecting = true;
        _connectStartTime = now;
        Serial.printf("[WiFi] Initiated WiFi.begin for: %s (ch=%u)\n", _pendingSsid.c_str(), _pendingChannel);
    }

    // 2. Check asynchronous non-blocking scan completion
    if (_scanning) {
        int16_t status = WiFi.scanComplete();
        if (status >= 0) {
            _scanning = false;
            _lastScanTime = now;
            if (status > 0) {
                populateScannedNetworks(status);
            } else {
                Serial.println("[WiFi] Background scan pass returned 0 results; retaining existing cache.");
            }
            WiFi.scanDelete();
        } else if (status == WIFI_SCAN_FAILED) {
            _scanning = false;
            _lastScanTime = now;
            WiFi.scanDelete();
            Serial.println("[WiFi] Asynchronous scan failed.");
        } else if (now - _scanStartTime > 8000) {
            _scanning = false;
            _lastScanTime = now;
            WiFi.scanDelete();
            Serial.println("[WiFi] Scan timed out after 8s.");
        }
    }

    // 3. Rate-limit station connection state machine checks to every 200ms
    if (now - _lastCheckTime < 200) return;
    _lastCheckTime = now;

    if (_connecting) {
        wl_status_t status = WiFi.status();
        if (status == WL_CONNECTED) {
            _connecting = false;
            _connectStatus = "connected";

            // Save confirmed working credentials into NVS storage
            SettingsManager::getInstance().setWifiSsid(_pendingSsid);
            SettingsManager::getInstance().setWifiPass(_pendingPass);
            SettingsManager::getInstance().setOfflineMode(false);

            // Keep SoftAP alive for 60s so user's phone browser receives the success response
            _apGracePeriodEnd = now + 60000;

            MDNS.begin(MDNS_HOSTNAME);
            MDNS.addService("http", "tcp", HTTP_PORT);
            SharedData::getInstance().setNetwork(true, WiFi.RSSI(), WiFi.localIP().toString().c_str());
            EventBus::getInstance().publish(EVENT_WIFI_CONNECTED, nullptr);

            char toastMsg[32];
            snprintf(toastMsg, sizeof(toastMsg), "IP: %s", WiFi.localIP().toString().c_str());
            RenderEngine::getInstance().showToast("WI-FI CONNECTED", toastMsg, COLOR_NEON_MINT, 5000);
            Serial.printf("[WiFi] Connected! IP: %s RSSI: %d dBm\n", WiFi.localIP().toString().c_str(), (int)WiFi.RSSI());
        } else if (now - _connectStartTime > 18000) { // 18 second connection timeout
            _connecting = false;
            _connectStatus = "timeout";
            WiFi.disconnect(false);
            RenderEngine::getInstance().showToast("TIMED OUT", "Could not connect", COLOR_NEON_RED, 4000);
            Serial.println("[WiFi] Connection timed out after 18s.");
            if (!_apActive) startApFallback();
        }
    } else if (WiFi.status() == WL_CONNECTED) {
        SharedData::getInstance().setNetwork(true, WiFi.RSSI(), WiFi.localIP().toString().c_str());
        // Once connected, shut down SoftAP after grace period to free radio bandwidth and power
        if (_apActive && _apGracePeriodEnd > 0 && now > _apGracePeriodEnd) {
            _dnsServer.stop();
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_STA);
            _apActive = false;
            _apGracePeriodEnd = 0;
            Serial.println("[WiFi] SoftAP grace period expired. Switched to Station-only mode.");
        }
    } else {
        // If not connected and not connecting, ensure SoftAP is available for recovery
        if (!_apActive && !_connecting && !_pendingConnect) {
            startApFallback();
        }
    }
}
