#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "src/display/render_engine.h"
#include "src/input/button_manager.h"
#include "src/core/event_bus.h"

// ==============================================================================
// ORBIT v2 — Polymorphic AppBase SDK
// ==============================================================================

struct AppMetadata {
    const char* id;          // Unique slug (e.g. "home", "climate")
    const char* title;       // Display title in carousel
    uint8_t iconIndex;       // 16x16 icon index in assets/icons.h
    bool backgroundRunning;  // If true, onUpdate() runs even when app is not active
};

class AppBase {
public:
    virtual ~AppBase() = default;

    // App Information
    virtual AppMetadata getMetadata() const = 0;

    // Lifecycle Hooks
    virtual void onInit() {}
    virtual void onEnter() {}
    virtual void onExit() {}

    // Frame Execution & Graphics
    virtual void onUpdate(uint32_t deltaMs) = 0;
    virtual void onRender(RenderEngine& gfx) = 0;

    // Input Dispatch (Return true if button event is consumed)
    virtual bool onButtonEvent(const ButtonEvent& evt) = 0;

    // Inter-System Event Receiver
    virtual void onEvent(EventType type, const void* payload) {}

    // WebSocket Remote Command Receiver
    virtual void onWsMessage(const String& cmd, const JsonVariantConst& data) {}
};
