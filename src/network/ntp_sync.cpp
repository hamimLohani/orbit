#include "src/network/ntp_sync.h"
#include "src/core/shared_data.h"
#include "src/core/settings_manager.h"
#include <WiFi.h>

// Reference New Moon Epoch: Jan 11, 2024, 11:57 UTC = 1704974220
#define NEW_MOON_REF_EPOCH 1704974220UL
#define SYNODIC_MONTH_SECS (29.53058867 * 86400.0)

NtpSync::NtpSync() 
    : _timeClient(_ntpUdp, "pool.ntp.org", 21600, 60000) {}

void NtpSync::init() {
    _tzOffsetMinutes = (int16_t)SettingsManager::getInstance().getAppInt("sys", "tz_mins", 360);
    _timeClient.setTimeOffset(_tzOffsetMinutes * 60);
    _timeClient.begin();
}

void NtpSync::setTimezoneOffset(int16_t offsetMinutes) {
    _tzOffsetMinutes = offsetMinutes;
    SettingsManager::getInstance().setAppInt("sys", "tz_mins", offsetMinutes);
    _timeClient.setTimeOffset(_tzOffsetMinutes * 60);
    forceSync();
}

void NtpSync::forceSync() {
    _synced = false;
    _lastSyncTime = 0;
}

void NtpSync::update() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    uint32_t now = millis();

    // 1. Initial sync or periodic sync every 30 minutes
    if (!_synced || (now - _lastSyncTime > 1800000)) {
        if (_timeClient.update()) {
            _synced = true;
            _lastSyncTime = now;
            Serial.printf("[NTP] Synchronized with NTP pool. Epoch: %lu\n", _timeClient.getEpochTime());
        }
    }

    // 2. Continuous per-second local clock ticker
    if (_synced && (now - _lastTickTime >= 1000)) {
        _lastTickTime = now;

        time_t localEpoch = (time_t)_timeClient.getEpochTime();
        struct tm* ti = gmtime(&localEpoch);
        if (ti) {
            char timeBuf[12];
            snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", ti->tm_hour, ti->tm_min, ti->tm_sec);

            char dateBuf[24];
            const char* const DAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
            const char* const MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
            snprintf(dateBuf, sizeof(dateBuf), "%s, %s %02d", DAYS[ti->tm_wday], MONTHS[ti->tm_mon], ti->tm_mday);

            bool night = (ti->tm_hour >= 22 || ti->tm_hour < 6);
            float phase = calculateMoonPhase((unsigned long)localEpoch);

            SharedData::getInstance().setTime(timeBuf, dateBuf, night, true);
            SharedData::getInstance().setMoonPhase(phase);
        }
    }
}

String NtpSync::getFormattedTime() const {
    if (_synced) {
        time_t epoch = (time_t)const_cast<NTPClient&>(_timeClient).getEpochTime();
        struct tm* ti = gmtime(&epoch);
        if (ti) {
            char buf[12];
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", ti->tm_hour, ti->tm_min, ti->tm_sec);
            return String(buf);
        }
    }
    return "12:00:00";
}

String NtpSync::getFormattedDate() const {
    if (_synced) {
        time_t epoch = (time_t)const_cast<NTPClient&>(_timeClient).getEpochTime();
        struct tm* ti = gmtime(&epoch);
        if (ti) {
            char buf[24];
            const char* const DAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
            const char* const MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
            snprintf(buf, sizeof(buf), "%s, %s %02d", DAYS[ti->tm_wday], MONTHS[ti->tm_mon], ti->tm_mday);
            return String(buf);
        }
    }
    return "Jan 01, 2026";
}

bool NtpSync::isNightTime() const {
    if (!_synced) return false;
    time_t epoch = (time_t)const_cast<NTPClient&>(_timeClient).getEpochTime();
    struct tm* ti = gmtime(&epoch);
    if (ti) {
        return (ti->tm_hour >= 22 || ti->tm_hour < 6);
    }
    return false;
}

float NtpSync::calculateMoonPhase(unsigned long epochTime) const {
    if (epochTime < NEW_MOON_REF_EPOCH) return 0.5f;
    double diff = (double)(epochTime - NEW_MOON_REF_EPOCH);
    double cycles = diff / SYNODIC_MONTH_SECS;
    double phase = cycles - (long)cycles;
    return (float)phase; // 0.0 to 1.0
}

float NtpSync::getMoonPhase() const {
    if (_synced) {
        return calculateMoonPhase(const_cast<NTPClient&>(_timeClient).getEpochTime());
    }
    return 0.5f;
}
