# Developer Guide: Adding New Apps to ORBIT

ORBIT is designed around a **modular micro-kernel architecture**. You never need to touch the core operating system or main loop to add a new application.

---

## 3-Step Quick Start

### Step 1: Create your Header (`src/apps/my_app.h`)

Create your class inheriting from `AppBase`:

```cpp
#pragma once
#include "core/app_base.h"

class MyApp : public AppBase {
public:
    MyApp();
    ~MyApp() override = default;

    // Metadata for the OS launcher
    AppMetadata getMetadata() const override {
        return {
            "my_app",      // Unique app ID string
            "My App",      // Title displayed in carousel & web app
            3,             // Icon index in sprite sheet
            false          // backgroundRunning (set true if background updates are required)
        };
    }

    void onInit() override;
    void onEnter() override;
    void onExit() override;
    void onUpdate(uint32_t deltaMs) override;
    void onRender(RenderEngine& gfx) override;
    bool onButtonEvent(const ButtonEvent& evt) override;
    void onEvent(EventType type, const void* payload) override;
    void onWsMessage(const String& cmd, const JsonVariantConst& data) override;

private:
    int _counter = 0;
};
```

---

### Step 2: Implement the Logic (`src/apps/my_app.cpp`)

Implement your app callbacks and use the `REGISTER_APP` macro:

```cpp
#include "apps/my_app.h"
#include "core/app_registry.h"
#include "display/theme.h"

// Register the app into the ORBIT carousel system automatically!
REGISTER_APP(MyApp);

MyApp::MyApp() {}

void MyApp::onInit() {
    // One-time initialization on boot
}

void MyApp::onEnter() {
    // Called when the user navigates into your app
    _counter = 0;
}

void MyApp::onExit() {
    // Called when user presses BACK to return to Home
}

void MyApp::onUpdate(uint32_t deltaMs) {
    // Frame updates, physics, animations
}

void MyApp::onRender(RenderEngine& gfx) {
    // Fill content area below status bar
    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);
    gfx.drawHeader("MY APP", 3);
    
    // Primary card follows standard UI grid (starts at y=40, width=112)
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 26, 112, 64, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 38, "Counter Value", COLOR_TEXT_MUTED, 1);
    
    char valBuf[16];
    snprintf(valBuf, sizeof(valBuf), "%d", _counter);
    gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 58, valBuf, COLOR_NEON_CYAN, 2);

    // Standard footer hint at SCREEN_HEIGHT - 10 (y=150)
    gfx.drawTextCentered(64, SCREEN_HEIGHT - 10, "UP / DOWN: Adjust", COLOR_TEXT_MUTED, 1);
}

bool MyApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK) {
        if (evt.button == BTN_UP) {
            _counter = min(10, _counter + 1);
            return true; // Event consumed
        } else if (evt.button == BTN_DOWN) {
            _counter = max(0, _counter - 1);
            return true; // Event consumed
        }
    }
    // Return false to let the system handle standard navigation (e.g. BACK returns to Home)
    return false;
}

void MyApp::onEvent(EventType type, const void* payload) {
    // React to system events (e.g., sound spikes, sensor ticks, notifications)
    if (type == EVENT_SOUND_SPIKE) {
        // Example: pulse animation on sound
    }
}

void MyApp::onWsMessage(const String& cmd, const JsonVariantConst& data) {
    // Handle remote commands from the Companion Web App
    if (cmd == "set_counter") {
        _counter = data["val"].as<int>();
    }
}
```

---

### Step 3: Compile and Flash

That's it! When you compile the firmware:
1. The app will automatically appear in the horizontal carousel launcher.
2. The companion web app will automatically detect it via `/api/apps`.
3. Navigation and system events will seamlessly route to your app.

---

## Available System Services

As an `AppBase` subclass, you have direct access to ORBIT's subsystems:

| Service | Accessor / Object | Purpose |
| :--- | :--- | :--- |
| **Graphics** | `gfx` (`RenderEngine`) | Primitives (`drawCircle`, `fillRect`), text, progress bars, bitmap icons, and toast notifications. |
| **Telemetry** | `SharedData::getInstance()` | Read live temperature, humidity, audio level, RSSI, and battery. |
| **Settings** | `SettingsManager::getInstance()` | Save and load persistent key-value pairs stored in NVS. |
| **Event Bus** | `EventBus::getInstance()` | Publish custom events or subscribe to system broadcasts. |
| **Lighting** | `RgbController::getInstance()` | Trigger WS2812 color modes, breathing pulses, and alert strobes. |
| **Indicator** | `IndicatorLed::getInstance()` | Trigger hardware notification flashes without blocking. |
| **SD Card** | `BitmapLoader::getSdMutex()` | Access files on the shared SPI bus safely using the FreeRTOS mutex. |
