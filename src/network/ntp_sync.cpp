#include "src/network/ntp_sync.h"
#include "src/core/shared_data.h"
#include <WiFi.h>

// Reference New Moon Epoch: Jan 11, 2024, 11:57 UTC = 1704974220
#define NEW_MOON_REF_EPOCH 1704974220UL
#define SYNODIC_MONTH_SECS (29.53058867 * 86400.0)

NtpSync::NtpSync() 
    : _timeClient(_ntpUdp, "pool.ntp.org", 0, 60000) {}

void NtpSync::init() {
    _timeClient.begin();
}

void NtpSync::update() {
    if (WiFi.status() != WL_CONNECTED) return;

    uint32_t now = millis();
    if (!_synced || (now - _lastSyncTime > 3600000)) { // Sync hourly
        if (_timeClient.update()) {
            _synced = true;
            _lastSyncTime = now;

            String timeStr = _timeClient.getFormattedTime();
            unsigned long epoch = _timeClient.getEpochTime();
            float phase = calculateMoonPhase(epoch);

            bool night = isNightTime();
            SharedData::getInstance().setTime(timeStr.c_str(), "Orbit Sync", night);
            SharedData::getInstance().setMoonPhase(phase);
        }
    } else {
        // Local second ticking
        String timeStr = _timeClient.getFormattedTime();
        SharedData::getInstance().setTime(timeStr.c_str(), nullptr, isNightTime());
    }
}

String NtpSync::getFormattedTime() const {
    if (_synced) {
        return const_cast<NTPClient&>(_timeClient).getFormattedTime();
    }
    return "12:00:00";
}

String NtpSync::getFormattedDate() const {
    return "ORBIT OS";
}

bool NtpSync::isNightTime() const {
    if (!_synced) return false;
    int hours = const_cast<NTPClient&>(_timeClient).getHours();
    return (hours >= 22 || hours < 6);
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
