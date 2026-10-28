#include "src/core/shared_data.h"
#include <cstring>

SharedData::SharedData() {
    _mutex = xSemaphoreCreateMutex();
}

SharedData::~SharedData() {
    if (_mutex) {
        vSemaphoreDelete(_mutex);
    }
}

void SharedData::init() {
    if (!_mutex) {
        _mutex = xSemaphoreCreateMutex();
    }
}

TelemetryData SharedData::getData() {
    TelemetryData copy;
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(15)) == pdTRUE) {
        copy = _data;
        xSemaphoreGive(_mutex);
    }
    return copy;
}

void SharedData::setClimate(float temp, float hum) {
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(15)) == pdTRUE) {
        _data.temperature = temp;
        _data.humidity = hum;
        // Heat Index simplified formula
        _data.heatIndex = temp + 0.33f * (hum / 100.0f * 6.105f * exp((17.27f * temp) / (237.7f + temp))) - 4.0f;
        xSemaphoreGive(_mutex);
    }
}

void SharedData::setWeather(float temp, float feelsLike, float hum, float windSpeed, uint16_t pressure, const char* condition, const char* city, const char* desc) {
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(15)) == pdTRUE) {
        _data.weatherValid = true;
        _data.outdoorTemp = temp;
        _data.outdoorFeelsLike = feelsLike;
        _data.outdoorHumidity = hum;
        _data.windSpeedMps = windSpeed;
        _data.pressureHpa = pressure;
        if (condition) {
            strncpy(_data.weatherCondition, condition, sizeof(_data.weatherCondition) - 1);
            _data.weatherCondition[sizeof(_data.weatherCondition) - 1] = '\0';
        }
        if (city) {
            strncpy(_data.weatherCity, city, sizeof(_data.weatherCity) - 1);
            _data.weatherCity[sizeof(_data.weatherCity) - 1] = '\0';
        }
        if (desc) {
            strncpy(_data.weatherDesc, desc, sizeof(_data.weatherDesc) - 1);
            _data.weatherDesc[sizeof(_data.weatherDesc) - 1] = '\0';
        }
        xSemaphoreGive(_mutex);
    }
}

void SharedData::setAudio(float rms, float peak, float db, const uint8_t* bins) {
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(15)) == pdTRUE) {
        _data.micRms = rms;
        _data.micPeak = peak;
        _data.micDb = db;
        if (bins) {
            memcpy(_data.fftBands, bins, FFT_NUM_BINS);
        }
        xSemaphoreGive(_mutex);
    }
}

void SharedData::setNoiseDistraction(bool alert) {
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(15)) == pdTRUE) {
        _data.noiseDistraction = alert;
        xSemaphoreGive(_mutex);
    }
}

void SharedData::setTime(const char* time, const char* date, bool isNight, bool synced) {
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(15)) == pdTRUE) {
        if (time) strncpy(_data.timeStr, time, sizeof(_data.timeStr) - 1);
        if (date) strncpy(_data.dateStr, date, sizeof(_data.dateStr) - 1);
        _data.isNightTime = isNight;
        _data.timeSynced = synced;
        xSemaphoreGive(_mutex);
    }
}

void SharedData::setMoonPhase(float phase) {
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(15)) == pdTRUE) {
        _data.moonPhase = phase;
        xSemaphoreGive(_mutex);
    }
}

void SharedData::setNetwork(bool connected, int8_t rssi, const char* ip) {
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(15)) == pdTRUE) {
        _data.wifiConnected = connected;
        _data.wifiRssi = rssi;
        if (ip) strncpy(_data.ipAddress, ip, sizeof(_data.ipAddress) - 1);
        xSemaphoreGive(_mutex);
    }
}

void SharedData::setSystemStatus(uint32_t heap, uint32_t uptime, const char* appId) {
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(15)) == pdTRUE) {
        _data.freeHeap = heap;
        _data.uptimeSec = uptime;
        if (appId) strncpy(_data.activeAppId, appId, sizeof(_data.activeAppId) - 1);
        xSemaphoreGive(_mutex);
    }
}

void SharedData::setLighting(uint8_t mode, uint32_t color, uint8_t brightness) {
    if (_mutex && xSemaphoreTake(_mutex, pdMS_TO_TICKS(15)) == pdTRUE) {
        _data.rgbMode = mode;
        _data.rgbColor = color;
        _data.brightness = brightness;
        xSemaphoreGive(_mutex);
    }
}
