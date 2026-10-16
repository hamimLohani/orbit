# ORBIT v2 — Engineering Specification & Architecture Blueprint
**Ambient Sensing & Companion Embedded OS for ESP32-S3**
*Version: 2.0.0 | Status: Production Master Specification*

---

## 1. Executive Summary & Vision

**ORBIT** is a next-generation ambient sensing companion and desktop embedded operating system running on the **ESP32-S3**. Unlike typical microcontroller sensor demos, ORBIT is built as a true **micro-kernel embedded OS**:
- **Cohesive Cyber/Astral Visual Identity**: Fluid UI running at 30–60 FPS on a 1.8" ST7735 TFT (128×160).
- **Extensible Card-Based App System**: Modular polymorphic application architecture where new applications can be created in minutes with zero modifications to core OS logic.
- **Bi-Directional Web Companion**: Single Page Application (SPA) communicating over low-latency WebSockets with live telemetry streaming, remote control, and client-side asset processing.
- **Ambient Intelligence & Companion Engine**: Procedural emotional entity that responds to environmental conditions (temperature, humidity, noise, time of day).
- **Real-Time DSP Audio Processing**: Continuous I2S DMA sampling with 16-band audio spectrum analyzer, sound-reactive lighting, and voice memo recording.

---

## 2. Hardware Architecture & Inventory

### 2.1 Hardware Inventory Table

| Component | Interface | Description & Purpose |
| :--- | :--- | :--- |
| **ESP32-S3 DevKit** | MCU Core | Dual-core Xtensa LX7 @ 240MHz, 8MB Flash, 512KB SRAM, 2/8MB PSRAM, Wi-Fi 802.11 b/g/n, BLE 5.0 |
| **ST7735 1.8" TFT** | SPI (Shared) | 128×160 resolution, 16-bit RGB565 color depth, hardware PWM backlight control |
| **MicroSD Slot** (on TFT) | SPI (Shared) | Dedicated Chip Select (`SD_CS`), stores wallpapers, custom icons, voice memos (`.wav`), logs, and configs |
| **INMP441 MEMS Mic** | I2S (DMA) | Omnidirectional digital microphone for ambient decibel monitoring, audio spectrum analysis, and voice memos |
| **DHT11 Sensor** | 1-Wire Digital | Non-blocking temperature (0–50°C) and relative humidity (20–90% RH) sampling |
| **WS2812 Addressable RGB** | GPIO (RMT/PWM) | Built-in RGB status and ambient mood lighting (breathe, disco, color cycle, notifications) |
| **Indicator Alert LED** | GPIO (Digital Out) | Dedicated alert beacon with hardware non-blocking flash patterns (heartbeat, SOS, alert strobe) |
| **5× Tactile Push Buttons** | GPIO (Debounced) | Navigation controls: `UP`, `DOWN`, `SELECT`, `BACK`, and `ACTION` (Quick Action / PTT) |

### 2.2 Central Pin Mapping (Centralized in `config.h`)

All pin assignments are strictly centralized in `include/config.h`. No hardcoded GPIO numbers are permitted anywhere in application or driver code.

```cpp
// ==========================================
// ORBIT v2 Hardware Pin Configuration
// ==========================================

// Shared SPI Bus (ST7735 Display + MicroSD Card)
#define PIN_SPI_MOSI          11
#define PIN_SPI_MISO          13
#define PIN_SPI_SCK           12

// ST7735 1.8" Display (128x160)
#define PIN_TFT_CS            10
#define PIN_TFT_DC            9
#define PIN_TFT_RST           14
#define PIN_TFT_BL            21   // PWM Backlight control (LEDC)

// MicroSD Card (Shared SPI, Dedicated CS)
#define PIN_SD_CS             47

// INMP441 I2S Digital MEMS Microphone
#define PIN_I2S_SCK           5    // BCLK (Bit Clock)
#define PIN_I2S_WS            4    // LRCLK (Word Select)
#define PIN_I2S_SD            6    // DOUT from mic (Data Out)

// Sensors & Indicators
#define PIN_DHT11             7    // Single-wire DHT11
#define PIN_RGB_LED           48   // Built-in WS2812 addressable RGB
#define PIN_INDICATOR_LED     2    // High-visibility alert LED

// 5-Button Navigation Matrix (Active LOW with internal pullups)
#define PIN_BTN_UP            1
#define PIN_BTN_DOWN          3
#define PIN_BTN_SELECT        18
#define PIN_BTN_BACK          17
#define PIN_BTN_ACTION        16   // PTT / Quick Action
```

### 2.3 SPI Bus Arbitration Protocol (ST7735 & MicroSD)

The ST7735 TFT and MicroSD card share the high-speed SPI bus (MOSI:11, MISO:13, SCK:12). To eliminate bus contention, corrupt pixels, and SD card timeouts:
1. **Dedicated Mutex Semaphore**: A FreeRTOS mutex (`spiBusMutex`) coordinates every SPI transaction.
2. **Explicit CS Isolation**: Before starting an SD transaction, `PIN_TFT_CS` is explicitly held `HIGH` (deselected). Before a display flush, `PIN_SD_CS` is held `HIGH`.
3. **SPI Transaction Scoping**: All accesses use `SPI.beginTransaction(settings)` and `SPI.endTransaction()`.

---

## 3. Modular Software Architecture & Subsystems

```
                                +-------------------------------------+
                                |          ORBIT Embedded OS          |
                                +-------------------------------------+
                                                   |
         +-----------------------------------------+-----------------------------------------+
         |                                         |                                         |
+------------------+                     +-------------------+                     +-------------------+
|  Hardware Layer  |                     |    Core Engine    |                     | Application Layer |
+------------------+                     +-------------------+                     +-------------------+
| - Display ST7735 |                     | - State Machine   |                     | - AppBase (SDK)   |
| - SD Storage     | <=== Bus Mutex ===> | - App Registry    | <=== Event Bus ===> | - Core Apps (12)  |
| - I2S INMP441    |                     | - Shared Telemetry|                     | - Future Plugins  |
| - DHT11 Sensor   |                     | - Settings (NVS)  |                     +-------------------+
| - RGB / Alert LED|                     +-------------------+                               |
| - Debounced Input|                               |                                         |
+------------------+                     +-------------------+                     +-------------------+
                                         |   Network Layer   | <== WebSocket / ==> | Companion Web App |
                                         +-------------------+     HTTP REST       +-------------------+
                                         | - WiFi / mDNS     |                     | - Realtime HUD    |
                                         | - NTP Time Sync   |                     | - Remote Control  |
                                         | - Async WebServer |                     | - Audio / AI STT  |
                                         | - Async WebSocket |                     | - Asset Pipeline  |
                                         | - OTA Updater     |                     +-------------------+
                                         +-------------------+
```

### 3.1 Design Principles
1. **Zero Blocking Delays**: No `delay()` anywhere in the main loop or apps. All timings are governed by FreeRTOS tasks and `millis()` delta timers.
2. **Dual-Core FreeRTOS Allocation**:
   - **Core 0**: High-bandwidth I2S audio continuous sampling, FFT processing, WiFi stack, and WebSocket networking.
   - **Core 1**: UI rendering (ST7735), button event dispatch, state machine, sensor management, and app lifecycle.
3. **Decoupled Event Bus (`EventBus`)**: System modules publish asynchronous events (`EVENT_SOUND_SPIKE`, `EVENT_SENSOR_UPDATE`, `EVENT_NOTIFICATION`, `EVENT_WEATHER_SYNC`), allowing any app to subscribe without tight coupling.
4. **Shared Telemetry Bus (`SharedData`)**: Thread-safe telemetry structure updated non-blockingly, accessible to all running apps simultaneously.

---

## 4. Enhanced & Exciting Feature Suite

In addition to the baseline features, ORBIT v2 integrates seven major advanced capabilities:

### 4.1 Aura Companion & Emotion Engine ("Orbit Pet")
- **Dynamic Animated Expressions**: High-framerate pixel art avatar rendered procedurally (Eye tracking, blinking, sleeping, happy, surprised, confused, dancing).
- **Ambient Reactivity**:
  - Temp < 18°C: Shivers with blue breath vapor.
  - Temp > 28°C: Sweats and pants with amber hue.
  - Mic Sound > 75dB: Perks ears up or grooves to rhythm.
  - Darkness / Night (NTP > 23:00): Enters deep slumber with floating `zZz` and soft breathing amber RGB.

### 4.2 Real-Time 16-Band DSP Audio Spectrum Visualizer
- **INMP441 DMA Continuous Sampling**: 16kHz / 32-bit sample collection via I2S DMA.
- **Hardware-Accelerated FFT**: Computes 16 frequency bins across 60Hz – 8kHz.
- **Visualizer Display Modes**:
  1. *Peak Bar Graph*: 16 responsive vertical bars with gravity-decay peak hold markers.
  2. *Phosphor VU Needle*: Dual stereo-style analog ballistic meters.
  3. *Cosmic Radial Pulse*: Concentric rings that expand with bass transients.
  4. *Oscilloscope Waveform*: Direct raw audio time-domain display.

### 4.3 Flow & Focus Productivity System (Smart Pomodoro + Noise Sentinel)
- **Visual Orbital Timer**: Crisp circular orbital progress ring counting down focus and rest cycles.
- **Noise Sentinel**: Continuously evaluates background noise. If ambient noise exceeds the user-configured focus threshold (e.g., 65dB) for more than 5 seconds, an alert pops up: *"High Distraction Alert"*.
- **RGB Mood Cues**: Soft breathing emerald for deep work, gentle lavender for break intervals, pulsing crimson for noise alerts.

### 4.4 Space & Astro Telemetry Dashboard
- **Procedural Moon Phase Engine**: Accurate lunar phase calculation (Waxing/Waning Crescent, Gibbous, Full, New) rendered with pixel shading and illumination percentage without needing external APIs.
- **ISS Live Tracker**: Fetches real-time International Space Station orbit coordinates via Open-Notify API, displaying distance, speed, and visible pass countdowns.

### 4.5 Hardware Voice Memo Recorder & AI Transcription Pipeline
- **One-Touch PTT Audio Capture**: Hold `BTN_ACTION` anywhere to immediately capture microphone audio directly to SD card as 16-bit 16kHz WAV (`/memos/memo_001.wav`).
- **Waveform HUD Overlay**: Live recording timer and audio level meter appear on screen during recording.
- **Companion AI Integration**: Web companion detects new memos over WebSocket, offers one-click in-browser audio playback, and triggers AI Speech-to-Text (Whisper / Gemini) for automated meeting notes and to-do extraction.

### 4.6 Retro Micro-Arcade (Orbit Lander)
- **Full Physics Mini-Game**: Built-in retro arcade game utilizing the 5 tactile buttons.
- Vector gravity simulation, landing pad detection, fuel consumption, and collision dynamics.
- High score persisted in NVS storage.

### 4.7 Universal Notification HUD & Live Ticker
- **Global Toast Notification Layer**: Sliding animated top-banner overlay that can display priority alerts over *any* active app without crashing or resetting the app state.
- **Live Stream Ticker**: Configurable ticker for live crypto/stock prices, GitHub stars, or desktop notifications pushed via REST webhook or WebSocket.

---

## 5. Complete App Catalog (12 Modular Apps)

| App ID | App Name | Display Icon | Key Responsibilities |
| :--- | :--- | :--- | :--- |
| `00` | **Home** | `ICON_HOME` | Primary dashboard: Orbiting ring animation, live clock, temperature, ambient decibel gauge, mood text. |
| `01` | **Aura** | `ICON_AURA` | Interactive virtual companion & emotion engine with ambient reactions. |
| `02` | **Spectrum** | `ICON_SPECTRUM`| 16-band real-time audio FFT analyzer and phosphor VU visualizer. |
| `03` | **Climate** | `ICON_CLIMATE` | DHT11 telemetry, comfort index, humidity gauge, and rolling 24-sample sparkline history. |
| `04` | **Focus** | `ICON_FOCUS` | Pomodoro focus timer with visual circular ring and background noise distraction sentinel. |
| `05` | **Recorder** | `ICON_MIC` | Voice memo audio recorder to SD card (`.wav`) with recording waveform HUD. |
| `06` | **Astro** | `ICON_ASTRO` | Lunar phase visualizer, ISS pass tracker, and space telemetry. |
| `07` | **Signal** | `ICON_SIGNAL` | Notification center, webhook push listener, and manual beacon trigger. |
| `08` | **Lights** | `ICON_LIGHTS` | WS2812 RGB lighting controller (Ambient, Disco, Flame, Breathe, Solid Picker). |
| `09` | **Lander** | `ICON_GAME` | Retro vector lunar lander physics arcade game. |
| `10` | **Network** | `ICON_WIFI` | Wi-Fi scanner, RSSI signal meter, NTP synchronization status, IP/mDNS info, OTA trigger. |
| `11` | **System** | `ICON_SYSTEM` | Display backlight slider, visual theme picker, SD card storage statistics, factory reset. |

---

## 6. Future-Proof App SDK Architecture

To ensure that adding new features in the future requires zero refactoring of the core OS, ORBIT introduces the **`AppBase` Polymorphic SDK**:

### 6.1 The `AppBase` Contract (`src/core/app_base.h`)

```cpp
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "display/render_engine.h"
#include "input/button_manager.h"
#include "core/event_bus.h"

struct AppMetadata {
    const char* id;          // Unique slug (e.g., "calc", "mqtt_switch")
    const char* title;       // Display title (e.g., "Calculator")
    uint16_t iconIndex;      // Index in system icon sheet
    bool backgroundRunning;  // If true, onUpdate() is invoked even when inactive
};

class AppBase {
public:
    virtual ~AppBase() {}

    // Core Lifecycle Hooks
    virtual AppMetadata getMetadata() const = 0;
    virtual void onInit() {}
    virtual void onEnter() {}
    virtual void onExit() {}

    // Rendering & Frame Execution
    virtual void onUpdate(uint32_t deltaMs) = 0;
    virtual void onRender(RenderEngine& gfx) = 0;

    // Input Handling (Return true if event was consumed)
    virtual bool onButtonEvent(const ButtonEvent& evt) = 0;

    // Decoupled Inter-System Event Receiver
    virtual void onEvent(EventType type, const void* payload) {}

    // WebSocket Remote Command Receiver
    virtual void onWsMessage(const String& cmd, const JsonVariantConst& data) {}
};
```

### 6.2 App Self-Registration Macro

Adding a new app to ORBIT in the future requires exactly **3 lines of registration**:

```cpp
// In your custom app file: my_custom_app.cpp
#include "apps/my_custom_app.h"
#include "core/app_registry.h"

// Self-registration macro automatically binds the app into the carousel at startup
REGISTER_APP(MyCustomApp);
```

### 6.3 Dynamic Carousel Navigation

The `AppRegistry` maintains the active app list, manages memory, executes smooth horizontal card transitions, and routes button presses to the current active app. Pressing `BTN_BACK` from within any app cleanly transitions back to the Home Dashboard.

---

## 7. Companion Web App Architecture (Single Page Application)

Served directly from ESP32-S3 LittleFS / SPIFFS or hosted externally via mDNS (`http://orbit.local`):

### 7.1 Web Application Subsystems
1. **Live HUD Dashboard**: WebSocket-synchronized telemetry cards for Temperature, Humidity, Mic dB, and Wi-Fi RSSI updated at 10Hz.
2. **Dynamic App Remote**: Automatically queries `/api/apps` from the device and generates interactive app launcher cards with active-state highlights.
3. **RGB Studio**: HTML5 color wheel, ambient brightness slider, and preset mode switcher (Breathe, Rainbow, Disco, Pulse, Solid).
4. **Voice Memo Hub**: Lists `.wav` recordings on the SD card, provides an HTML5 audio player with waveform preview, and provides a direct hook to Whisper AI / Gemini API.
5. **Asset Studio**: Client-side canvas utility allowing users to upload any PNG/JPEG image. The browser automatically crops, resizes to 128×160, converts to 16-bit RGB565 BMP, and streams the binary over WebSocket straight into SD card `/images/wallpapers/`.
6. **Notification Transmitter**: Form to push high-priority toast messages to the device screen with custom RGB chime colors.
7. **Firmware OTA Manager**: Web-based binary upload form for over-the-air firmware upgrades.

### 7.2 WebSocket Protocol Specification

All WebSocket packets use lightweight JSON messages with a standard structure:

```json
// Telemetry Packet (Device -> Client, 10Hz)
{
  "type": "telemetry",
  "d": {
    "temp": 24.5,
    "hum": 58.0,
    "mic": 42.1,
    "heap": 184520,
    "wifi_rssi": -58,
    "active_app": "home",
    "rgb_mode": "ambient",
    "uptime": 3600
  }
}

// Command Packet (Client -> Device)
{
  "cmd": "switch_app",
  "app": "spectrum"
}

// Push Notification Packet (Client -> Device)
{
  "cmd": "toast",
  "title": "Meeting in 5m",
  "body": "Design Sprint Review",
  "duration_ms": 5000,
  "rgb_color": "#00FFCC"
}
```

---

## 8. Directory & File Blueprint

```
orbit/
├── platformio.ini                  # PlatformIO build configuration
├── orbit.ino                       # Arduino IDE / CLI entry wrapper
├── include/
│   ├── config.h                    # Pinouts, timings, hardware constants
│   └── assets/
│       ├── icons.h                 # 16x16 pixel icon bitmaps
│       └── splash_logo.h           # Boot logo bitmap
├── src/
│   ├── main.cpp                    # FreeRTOS setup(), Core 0/1 orchestration
│   ├── core/
│   │   ├── app_base.h              # Polymorphic App interface
│   │   ├── app_registry.h/.cpp     # Dynamic app registry & carousel
│   │   ├── event_bus.h/.cpp        # Decoupled publish/subscribe bus
│   │   ├── shared_data.h/.cpp      # Thread-safe telemetry struct
│   │   └── settings_manager.h/.cpp # NVS persistent settings store
│   ├── display/
│   │   ├── render_engine.h/.cpp    # ST7735 graphics wrapper, primitives, toasts
│   │   ├── bitmap_loader.h/.cpp    # SD & Flash BMP image loader (with SPI mutex)
│   │   └── theme.h                 # Cohesive 16-bit RGB565 palette
│   ├── input/
│   │   └── button_manager.h/.cpp   # 5-button debounced input engine
│   ├── sensors/
│   │   ├── sensor_manager.h/.cpp   # Non-blocking sensor scheduler
│   │   ├── dht11_sensor.h/.cpp     # DHT11 temperature & humidity driver
│   │   └── mic_sensor.h/.cpp       # INMP441 I2S DMA sampler & FFT engine
│   ├── lighting/
│   │   ├── rgb_controller.h/.cpp   # WS2812 addressable RGB lighting engine
│   │   └── indicator_led.h/.cpp    # Dedicated alert LED non-blocking flasher
│   ├── network/
│   │   ├── wifi_manager.h/.cpp     # WiFi station & AP manager
│   │   ├── ntp_sync.h/.cpp         # Real-time clock synchronizer
│   │   ├── websocket_server.h/.cpp # Bi-directional JSON WebSocket server
│   │   ├── web_server.h/.cpp       # AsyncWebServer serving LittleFS & REST
│   │   └── ota_handler.h/.cpp      # ArduinoOTA and Web OTA updates
│   ├── apps/
│   │   ├── app_template/           # Copy-paste template for future apps
│   │   │   ├── template_app.h
│   │   │   └── template_app.cpp
│   │   ├── home_app.h/.cpp         # Live orbiting home dashboard
│   │   ├── companion_app.h/.cpp    # Aura interactive emotion engine
│   │   ├── spectrum_app.h/.cpp     # 16-band audio FFT visualizer
│   │   ├── climate_app.h/.cpp      # DHT11 environmental monitor
│   │   ├── focus_app.h/.cpp        # Pomodoro & Noise Sentinel
│   │   ├── recorder_app.h/.cpp     # Voice memo SD WAV recorder
│   │   ├── astro_app.h/.cpp        # Space telemetry & Moon phase
│   │   ├── signal_app.h/.cpp       # Alert center & webhooks
│   │   ├── lights_app.h/.cpp       # RGB mood lighting controller
│   │   ├── lander_app.h/.cpp       # Retro Lunar Lander physics game
│   │   ├── network_app.h/.cpp      # Wi-Fi scanner & network info
│   │   └── system_app.h/.cpp       # Settings & brightness controls
├── data/                           # LittleFS Web Assets
│   └── web/
│       ├── index.html              # Modern glassmorphic Web Companion
│       ├── style.css               # Responsive cyber aesthetic CSS
│       └── app.js                  # WebSocket client & asset converter
└── docs/
    └── ADDING_NEW_APPS.md          # 5-minute tutorial for future developers
```

---

## 9. Developer Guide: How to Add a New App in 5 Minutes

ORBIT makes adding future apps effortless:

1. **Create the files**: Create `src/apps/my_new_app.h` and `src/apps/my_new_app.cpp` (or copy `src/apps/app_template/`).
2. **Implement `AppBase`**:
   ```cpp
   #pragma once
   #include "core/app_base.h"

   class MyNewApp : public AppBase {
   public:
       AppMetadata getMetadata() const override {
           return { "new_app", "New App", 5, false };
       }
       void onUpdate(uint32_t deltaMs) override {}
       void onRender(RenderEngine& gfx) override {
           gfx.fillScreen(THEME_BG);
           gfx.drawTextCentered(64, 70, "Hello ORBIT!", THEME_CYAN);
       }
       bool onButtonEvent(const ButtonEvent& evt) override {
           return false; // Return true if handled
       }
   };
   ```
3. **Register it**: In `src/apps/my_new_app.cpp`, register it with the system:
   ```cpp
   #include "apps/my_new_app.h"
   #include "core/app_registry.h"

   REGISTER_APP(MyNewApp);
   ```
4. **Compile**: The app will automatically appear in the carousel, be reachable via button navigation, and show up in the web app launcher. No core files need to be touched!
