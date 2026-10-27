#pragma once
#include <Arduino.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <time.h>

// ==============================================================================
// ORBIT v2 — NTP Time Synchronization & Astro Calculator
// Synchronizes current time & calendar date over Wi-Fi
// ==============================================================================

class NtpSync {
public:
    static NtpSync& getInstance() {
        static NtpSync instance;
        return instance;
    }

    void init();
    void update(); // Called regularly from Core 0 network task

    String getFormattedTime() const;
    String getFormattedDate() const;
    bool isNightTime() const;
    float getMoonPhase() const; // 0.0 = New Moon, 0.5 = Full Moon
    bool isTimeSynced() const { return _synced; }

    void setTimezoneOffset(int16_t offsetMinutes);
    int16_t getTimezoneOffset() const { return _tzOffsetMinutes; }
    void forceSync();

private:
    NtpSync();
    ~NtpSync() = default;

    float calculateMoonPhase(unsigned long epochTime) const;

    WiFiUDP _ntpUdp;
    NTPClient _timeClient;
    bool _synced = false;
    uint32_t _lastSyncTime = 0;
    uint32_t _lastTickTime = 0;
    int16_t _tzOffsetMinutes = 360; // Default UTC+6:00
};
