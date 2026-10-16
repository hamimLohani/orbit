#include "src/sensors/sensor_manager.h"
#include "src/sensors/dht11_sensor.h"
#include "src/sensors/mic_sensor.h"
#include "src/core/shared_data.h"
#include "src/core/event_bus.h"

void SensorManager::init() {
    Dht11Sensor::getInstance().init();
    MicSensor::getInstance().init();
}

void SensorManager::updateSensors() {
    // 1. Update DHT11
    if (Dht11Sensor::getInstance().update()) {
        float temp = Dht11Sensor::getInstance().getTemperature();
        float hum = Dht11Sensor::getInstance().getHumidity();
        SharedData::getInstance().setClimate(temp, hum);
        EventBus::getInstance().publish(EVENT_SENSOR_UPDATE, nullptr);
    }

    // 2. Mic sensor is polled in Core 0 or during loop
    MicSensor::getInstance().update();

    uint32_t now = millis();
    if (now - _lastMicUpdate >= SENSOR_POLL_MIC_MS) {
        _lastMicUpdate = now;
        SharedData::getInstance().setAudio(
            MicSensor::getInstance().getRms(),
            MicSensor::getInstance().getPeak(),
            MicSensor::getInstance().getDb(),
            MicSensor::getInstance().getFftBands()
        );
    }
}
