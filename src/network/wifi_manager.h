#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <vector>
#include "include/config.h"

// ==============================================================================
// ORBIT v2 — Wi-Fi, Captive Portal DNS & Network Provisioning Manager
// ==============================================================================

struct ScannedNetwork {
    String ssid;
    int32_t rssi;
    bool secure;
    int32_t channel;
};

class WifiManager {
public:
    static WifiManager& getInstance() {
        static WifiManager instance;
        return instance;
    }

    void init();
    void update(); // Called from Core 0 network task

    bool isConnected() const;
    bool isApActive() const { return _apActive; }
    bool isConnecting() const { return _connecting; }
    int8_t getRssi() const;
    String getIpAddress() const;
    String getSsid() const;
    String getSoftApSsid() const { return DEFAULT_AP_SSID; }
    String getSoftApIp() const;

    void connect(const String& ssid, const String& pass);
    void disconnect();
    void forgetWifi();
    void startApFallback();

    // Asynchronous Wi-Fi Scanning
    void startScan();
    void requestScan() { _scanRequested = true; }
    bool isScanning() const { return _scanning; }
    const std::vector<ScannedNetwork>& getScannedNetworks() const { return _scannedNetworks; }
    String getScannedNetworksJson();

private:
    WifiManager() = default;
    ~WifiManager() = default;

    void startStation();
    void handleBackgroundScan();

    DNSServer _dnsServer;
    bool _apActive = false;
    bool _connecting = false;
    bool _scanning = false;
    bool _scanRequested = false;
    uint32_t _scanStartTime = 0;
    uint32_t _lastScanTime = 0;
    uint32_t _lastCheckTime = 0;
    uint32_t _connectStartTime = 0;
    uint32_t _apGracePeriodEnd = 0;
    std::vector<ScannedNetwork> _scannedNetworks;
};
