#include "src/core/event_bus.h"

void EventBus::subscribe(EventType type, EventCallback callback) {
    if (callback) {
        _subscribers.push_back({type, callback});
    }
}

void EventBus::publish(EventType type, const void* payload) {
    for (const auto& sub : _subscribers) {
        if (sub.type == EVENT_NONE || sub.type == type) {
            sub.callback(type, payload);
        }
    }
}
