#pragma once
#include <Arduino.h>
#include <functional>
#include <vector>

// ==============================================================================
// ORBIT v2 — Decoupled Publish-Subscribe Event Bus
// ==============================================================================

enum EventType {
    EVENT_NONE = 0,
    EVENT_TICK_1S,
    EVENT_SENSOR_UPDATE,
    EVENT_SOUND_SPIKE,
    EVENT_NOISE_ALERT,
    EVENT_BUTTON_PRESS,
    EVENT_WIFI_CONNECTED,
    EVENT_WIFI_DISCONNECTED,
    EVENT_NOTIFICATION_ARRIVED,
    EVENT_APP_SWITCHED,
    EVENT_RECORDING_START,
    EVENT_RECORDING_STOP,
    EVENT_CUSTOM = 100
};

struct NotificationPayload {
    char title[32];
    char body[64];
    uint16_t durationMs;
    uint32_t rgbColor;
};

typedef std::function<void(EventType type, const void* payload)> EventCallback;

class EventBus {
public:
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    // Subscribe to events. Pass EVENT_NONE to receive all events.
    void subscribe(EventType type, EventCallback callback);

    // Publish an event synchronously or from interrupt/task
    void publish(EventType type, const void* payload = nullptr);

private:
    EventBus() = default;
    ~EventBus() = default;

    struct Subscription {
        EventType type;
        EventCallback callback;
    };

    std::vector<Subscription> _subscribers;
};
