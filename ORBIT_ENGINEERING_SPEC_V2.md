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
| **ESP32-S3 DevKit** | MCU Core | Dual-core Xtensa LX7 @ 240MHz, 8MB Flash, 512KB SRAM, 8MB Octal PSRAM (OPI, active), Wi-Fi 802.11 b/g/n, BLE 5.0 |
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
#define PIN_BTN_UP            1    // Directional UP
#define PIN_BTN_DOWN          3    // Directional DOWN
#define PIN_BTN_LEFT          17   // Directional LEFT (was Back)
#define PIN_BTN_RIGHT         18   // Directional RIGHT (was Select)
#define PIN_BTN_ENTER         16   // Enter / Action (1s=1 Step Back, 3s=Home)
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
| - SD Storage     | <=== Bus Mutex ===> | - App Registry    | <=== Event Bus ===> | - Core Apps (13)  |
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

### 3.1 Design Principles & FreeRTOS Microkernel Architecture
1. **Zero Blocking Delays**: No `delay()` anywhere in the main execution paths. All timings are governed by FreeRTOS tasks, `vTaskDelayUntil()`, and non-blocking timers.
2. **6 Discrete FreeRTOS Tasks with Dual-Core Affinity**:
   - **Core 0**: `AudioTask` (Pri 4, 16kHz I2S DMA & FFT), `NetworkTask` (Pri 1, WiFi/WebSockets/NTP/OTA).
   - **Core 1**: `InputTask` (Pri 4, Debounced Button Queue), `DisplayTask` (Pri 3, 30 FPS double-buffered UI), `LightingTask` (Pri 2, WS2812/LED animations), `SensorTask` (Pri 2, DHT11 climate).
3. **Decoupled Event Bus (`EventBus`)**: System modules publish asynchronous events (`EVENT_SOUND_SPIKE`, `EVENT_SENSOR_UPDATE`, `EVENT_NOTIFICATION`, `EVENT_WEATHER_SYNC`), allowing any app to subscribe without tight coupling.
4. **Shared Telemetry Bus (`SharedData`)**: Thread-safe telemetry structure guarded by FreeRTOS mutex with bounded timeouts.
5. **Detailed Operating System Concept Analysis**: See [`docs/OS_CONCEPTS.md`](docs/OS_CONCEPTS.md) for the complete OS theoretical mapping, Coffman deadlock verification, and task scheduling matrix.

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

### 4.2 Environmental & Climate Telemetry
- **DHT11 Dual Real-Time Telemetry**: Real-time temperature (°C/°F) and relative humidity (%) acquisition.
- **Microclimate Analysis**: Auto-calculated heat index and thermal comfort classification pill.
- **24-Point Live Trend Sparkline**: Symmetrical display metrics with live auto-scaled historical curve.

### 4.3 Flow & Focus Productivity System (Smart Pomodoro + Noise Sentinel)
- **Visual Orbital Timer**: Crisp circular orbital progress ring counting down focus and rest cycles.
- **Interactive Timer Edit System**:
  - Direct quick adjustments from IDLE: `LEFT` / `RIGHT` adjust Focus session duration by ±5 minutes on the fly with real-time dial and NVS persistence.
  - Dedicated **Edit Timer Mode** (`UP` / `DOWN` in IDLE): 3-card configuration studio with active row highlighting:
    - **Focus Time Slider**: Custom work interval from 1 to 90 minutes with 104px progress bar indicator.
    - **Short Break Slider**: Custom rest interval from 1 to 30 minutes with 104px progress bar indicator.
    - **Quick Presets**: Instant one-click presets (`25m POMO`, `15m SPRINT`, `45m DEEP`, `60m POWER`).
  - Saved preferences stored in NVS flash via `SettingsManager` (`pomo_focus`, `pomo_break`) and restored at boot.
- **Noise Sentinel**: Continuously evaluates background noise. If ambient noise exceeds the user-configured focus threshold (e.g., 65dB) for more than 4 seconds, an alert pops up: *"High Distraction Alert"*.
- **RGB Mood Cues**: Soft breathing emerald for deep work, gentle lavender for break intervals, pulsing crimson for noise alerts.

### 4.4 Space & Astro Telemetry Dashboard
- **Procedural Moon Phase Engine**: Accurate lunar phase calculation (Waxing/Waning Crescent, Gibbous, Full, New) rendered with pixel shading and illumination percentage without needing external APIs.
- **ISS Live Tracker**: Fetches real-time International Space Station orbit coordinates via Open-Notify API, displaying distance, speed, and visible pass countdowns.

### 4.5 Hardware Voice Memo Recorder & Voice-to-Text Studio
- **Multi-Option Voice Recording Architecture (`RecorderApp`)**:
  - `MEMO_MODE_VOICE_TO_TEXT` (Default): Captures speech and automatically pushes audio metadata via WebSocket to the Companion Web Dashboard, triggering live speech-to-text transcription ready for one-click clipboard copying.
  - `MEMO_MODE_AUDIO_WAV`: Standard high-fidelity 16kHz 16-bit mono PCM audio recording stored locally on MicroSD card (`/memos/memo_xxxx.wav`).
  - `MEMO_MODE_AI_TASKS`: Voice notes destined for automatic AI summarization and action item extraction.
- **On-Device Option Switcher**:
  - Card 0 on TFT display allows cycling through options using `LEFT` / `RIGHT` buttons with neon color accents and descriptive mode tags.
  - Card 1 renders pulsing recording indicators, elapsed duration (`00:00.0`), and live mic dB level progress bar.
  - Card 2 displays MicroSD free storage and live Wi-Fi Web status (`WEB:ONLINE` / `WEB:OFFLINE`).
- **Companion AI & Web Speech Transcription**:
  - Real-time WebSocket event dispatch (`memo_event` start/stop) notifies the web companion immediately when recording completes.
  - In-browser **Voice-to-Text Studio** renders live transcripts with a high-visibility **"Copy to Clipboard"** button, live browser dictation (Web Speech API), `.txt` export, and toast push back to the device screen.
  - Complete SD Card recording library with HTML5 playback, individual file transcription, WAV downloads, and file deletion (`POST /api/recordings/delete`).

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
| `02` | **Climate** | `ICON_CLIMATE` | DHT11 dual symmetrical telemetry cards, live humidity progress bar, comfort badge pill, and 24-point auto-scaled trend sparkline. |
| `03` | **Focus** | `ICON_FOCUS` | Pomodoro focus timer with visual circular ring, background noise distraction sentinel, and interactive Timer Edit System (Focus duration 1-90m, Break 1-30m, and Quick Presets). |
| `04` | **Recorder** | `ICON_MIC` | Voice memo audio recorder to SD card (`.wav`) with recording waveform HUD. |
| `05` | **Astro** | `ICON_ASTRO` | Lunar phase visualizer, ISS pass tracker, and space telemetry. |
| `06` | **Signal** | `ICON_SIGNAL` | Notification center, webhook push listener, and manual beacon trigger. |
| `07` | **Lights** | `ICON_LIGHTS` | Dedicated WS2812 RGB lighting controller with instant Power toggle, mode cycling (Ambient, Disco, Breathe, Rainbow, Solid, Alert), brightness bar, and custom solid RGB Studio mixer with granular R, G, B (0-255) channel sliders and live preview swatch. |
| `08` | **Lander** | `ICON_GAME` | Retro vector lunar lander physics arcade game. |
| `09` | **Network** | `ICON_WIFI` | Dual-page network hub: Page 1 (STA/AP connection, IP/mDNS, NTP time) and Page 2 (Over-The-Air Flasher status, Web Flasher URL `http://orbit.local/update`, real-time TFT progress bar during Wi-Fi firmware flashing). |
| `10` | **System** | `ICON_SYSTEM` | Display backlight slider, 8-page hardware diagnostics with zero overlap (OS Platform & Backlight, Processor Silicon, Device ID & MAC, SRAM Heap with free %, External PSRAM with free %, SPI Flash ROM & Partition, Storage: LittleFS & MicroSD with free %, and TCP/IP Network). |
| `11` | **Snake**  | `ICON_SNAKE`  | Full port of Snake CLI: Classic, Time Attack, Survival, Zen modes, 6 power-ups/hazards, anti-suicide FIFO queue, persistent high scores. |

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

### 6.3 Dynamic Carousel Navigation & Button Routing Contract

The `AppRegistry` enforces a strict hierarchical navigation hierarchy to prevent accidental app-switching:
1. **Inside an Active App (`_activeIndex != 0`, `_inCarousel == false`)**:
   - Directional buttons `BTN_LEFT` and `BTN_RIGHT` belong strictly to the active app for internal controls (e.g., steering the snake, adjusting brightness/color palette, navigating diagnostic pages, or lander thrusters).
   - If an active app does not consume `LEFT` or `RIGHT`, the buttons perform no action. **Pressing `L/R` inside an app will NEVER change apps or switch menus.**
2. **Backing to the Launcher Menu**:
   - Holding `BTN_ENTER` for 1.0s (`BUTTON_HOLD_1S`) executes **1 step back**:
     - *In-App Sub-states*: Allows active apps to step back internally (e.g. game over / playing -> game menu, subpages -> page 0).
     - *App Root Level*: Backs out of the active app into the **OS Launcher Menu Carousel** (`_inCarousel = true`), highlighting the active app's card.
3. **Menu Option Selection**:
   - Once backed out to the Launcher Menu (`_inCarousel == true`), **`BTN_LEFT` and `BTN_RIGHT` change the selected menu option (`_carouselIndex`)**.
   - Pressing `BTN_ENTER` opens and activates the selected app.
   - Holding `BTN_ENTER` for 1.0s closes the Launcher Menu and returns to the Home Dashboard.
4. **Instant Home Return**:
   - Holding `BTN_ENTER` for 3.0s (`BUTTON_HOLD_3S`) returns directly to the Home Dashboard from anywhere across the entire operating system unconditionally.

### 6.4 Display Viewport & Coordinate Standards (128x160 Portrait)

All apps and render routines adhere to a unified UI coordinate grid to prevent shape overlapping and text truncation:
- **Display Resolution**: 128 (W) × 160 (H) pixels.
- **Top Status Bar**: `y = 0` to `y = 13` (`STATUS_BAR_HEIGHT = 14`), horizontal divider at `y = 13`.
- **App Header (`drawHeader`)**:
  - Icon: `x = 4`, `y = STATUS_BAR_HEIGHT + 3` (17..32)
  - Title: `x = 24`, `y = STATUS_BAR_HEIGHT + 7` (21..28)
  - Header Divider Line: `y = STATUS_BAR_HEIGHT + 23` (37)
- **Primary Content Card (Card 1)**:
  - Standard Placement: `x = 6..8`, `y = STATUS_BAR_HEIGHT + 25..26` (39..40), `width = 112..116`, `height = 42..48` (ends at `y = 82..88`).
  - Leaves a clean margin below the header divider line (`y = 37`).
- **Secondary Content Card (Card 2)**:
  - Standard Placement: `x = 6..8`, `y = STATUS_BAR_HEIGHT + 75..76` (89..90), `width = 112..116`, `height = 42..48` (ends at `y = 132..137`).
  - Leaves a clean 2..4px gap below Card 1.
- **Bottom Navigation Footer & Menu Navigation Standards**:
  - **Single-Line Footers**: Placed at `y = 146..148` (e.g. `SystemApp`, `LightsApp`, `ClimateApp`), leaving a generous 9..10px clear air gap below content cards (which end at `y <= 137`). Max text length at size 1: **19 characters** (114px width, centered at `x = 64` with 7px margins on display edges to prevent bezel clipping).
  - **Two-Line Menu Footers**: In interactive menus (`AppRegistry` Carousel Launcher and `SnakeApp` Retro Menu), content cards and badge boxes terminate cleanly at `y <= 108..120`:
    - Line 1 placed at `y = 127..129` (e.g. `< L / R >  OK: Open` or `U/D:Nav   L/R:Edit`), length <= 19 chars (114px width, 7px side margins).
    - Line 2 placed at `y = 141..143` (e.g. `1s: Back   3s: Home` or `OK:Play   1s:Back`), length <= 19 chars (114px width, 7px side margins).
    - In `AppRegistry`, line 2 is dynamically guarded when voice recording HUD banner is active, preventing overlay collision.
- **Game Over & Pause Screen UI Standards**:
  - **Full-Screen Game Over Pages**: Games transition into a clean, dedicated full-page Game Over screen rather than drawing opaque popups over cluttered arena grids.
  - **Summary Hero Card**: `x = 8`, `y = 40`, `width = 112`, `height = 76` (terminates cleanly at `y = 116`). Contains death cause banner (`COLOR_NEON_RED`), final score in size 2 font (`COLOR_NEON_CYAN` or `COLOR_NEON_AMBER`), record / high-score celebration indicator, and active mode/difficulty summary. Border accent flashes `COLOR_NEON_AMBER` on record breaks.
  - **Action Controls**: `[ OK: PLAY AGAIN ]` at `y = 127` and `<: Menu   1s: Back` at `y = 141`, with minimum 10px side margins to prevent bezel clipping. Directional buttons (`LEFT`, `DOWN`, `UP`) and 1s hold on Enter cleanly return to the game menu.
  - **Full-Screen Paused Pages**: Rather than drawing floating modal overlays over cluttered arena grids, the game transitions into a clean, dedicated full-page pause state (`GAME PAUSED` header). It renders an active session summary card (`x = 8`, `y = 40`, `width = 112`, `height = 76`), showcasing current score in size 2 font (`COLOR_NEON_CYAN`), live snake length or time attack remaining, and active mode/difficulty. Action footer provides `[ OK: RESUME GAME ]` (`COLOR_NEON_MINT`) and `<: Menu   1s: Back` (`COLOR_TEXT_MUTED`) with 10px side margins. Directional buttons (`LEFT`, `DOWN`, `UP`) and 1s hold on Enter cleanly return to the menu, while 3s hold on Enter returns Home.
- **Dial / Circular Gauges**:
  - Gauges with centers at `y = 58..62` must constrain radii to `r <= 21` pixels so circumference arcs never intersect the header divider line (`y = 37`).
  - Needle meters (e.g. VU ballistics) must draw upper sweep arcs only rather than 360° full circles to prevent crossing bottom footer text.
- **Persistent Hardware Backlight & RGB Retention**:
  - Display backlight duty cycle (0-100%) and RGB LED brightness (0-255) are stored in ESP32 NVS flash preferences (`SettingsManager`) and automatically restored during `RenderEngine::init()` and `RgbController::init()`, preserving user brightness preferences across power cycles and reboots.
  - Backlight duty mapping enforces a hardware duty floor (`TFT_BACKLIGHT_MIN_DUTY = 20` out of 255) at 0% brightness, guaranteeing that 0% dims the screen to a comfortable low-light level without ever completely shutting off the screen or blacking out navigation.
- **Exclusive Isolated RGB Lighting Control Architecture**:
  - WS2812 addressable RGB lighting (`PIN_RGB_LED` GPIO 48) is strictly handled and controlled within the dedicated **Lights App** (`LightsApp`) and the Web Companion RGB Studio.
  - Default boot state defaults to `RGB_OFF` (`0`), preventing the light from turning on unsolicited after boot or reset.
  - All non-lighting applications (`SnakeApp`, `LanderApp`, `FocusApp`, `CompanionApp`, `SignalApp`) and incoming toast notifications are completely decoupled from the RGB LED. No other app can illuminate, override, or flash the RGB neo-pixel.
  - In `RgbController`, any flash triggers are strictly ignored while the light is in `RGB_OFF` mode, guaranteeing that turning off the light in the Lights section keeps the LED completely dark across the entire OS.
- **Rearranged Lighting Architecture & Overlap-Free Typography**:
  - **On-Device Lights App (`LightsApp`)**:
    - **Card 0 (Mode & State Header)** (`y = 39..87`, height 48): Features 35px separation between the left Mode title (`> MODE` at `x = 14`, width 36px) and the right-aligned status badge (`xRight = 114`, e.g. `02/10 [ON]` or `[OFF]`), completely eliminating font collisions.
    - **Card 1 (RGB Brightness)** (`y = 91..136`, height 45): Left label `> RGB LEVEL` with right-aligned percentage (`100%`) separated by a clean 16px safety margin, 100px progress bar at `y = 110`, and navigation hint at `y = 123`.
    - **Solid Studio Mixer**: Channel row coordinates (`BRT`, `RED`, `GRN`, `BLU` at `y = 71, 87, 103, 119`) with numeric readouts right-aligned at `xRight = 56` and progress bars from `x = 60..112`, preventing numerical overlap with channel labels.
  - **Web Companion RGB Studio**:
    - **Master Power Action**: Dedicated high-visibility `⏻ Power Off RGB` action button with neon-red active indicator at the panel header.
    - **Categorized Mode Options**: 10 light modes organized into structured, responsive category cards:
      - *Dynamic & Reactive* (4 modes): Ambient Climate (Temp-reactive), Sound Disco (FFT beat visualizer), Disco Alert (Party beat strobe), Pulse Breathe (Sinusoidal glow).
      - *Atmospheric & Mood* (6 modes): Rainbow Cycle (Fluid HSV), Aurora Lights (Borealis wave), Warm Candle (Campfire flame), Police Siren (Red/Blue strobe), Critical Alert (Red warning), Solid Custom (Manual color mixer).
    - **Studio Controls**: Separate dedicated sliders for Hardware RGB LED Brightness (`#rgb-bright-slider`), Custom Color Palette with 8 vibrant neon preset swatches, and Display Screen Backlight (`#display-bright-slider`).
- **Custom Solid RGB Studio Mixer Controls (`LightsApp`)**:
  - In `RGB_SOLID` ("CUSTOM SOLID") mode, the app switches from the standard 2-card layout into the high-precision 5-field **RGB Studio Mixer**:
    - **Card 0 (Mode & Live Preview Swatch)** (`y = 41..66`, height 25): Shows Mode title on the left and a live, real-time 32×19px rounded RGB565 color preview swatch on the right with a crisp white border, instantly reflecting blended channel outputs.
    - **Card 1 (Studio Mixer Sliders)** (`y = 68..136`, height 68): Houses 4 individual 50px pill-slider rows with numeric readouts and color-coded indicator tracks:
      - **Field 1 (`BRT`)**: Overall LED brightness scaling (0–100%, 0–255).
      - **Field 2 (`RED`)**: Red channel intensity (0–255, step ±15 on `L/R`, quick toggle 0/128/255 on `OK`) with `COLOR_NEON_RED` track.
      - **Field 3 (`GRN`)**: Green channel intensity (0–255, step ±15 on `L/R`, quick toggle 0/128/255 on `OK`) with `COLOR_NEON_MINT` track.
      - **Field 4 (`BLU`)**: Blue channel intensity (0–255, step ±15 on `L/R`, quick toggle 0/128/255 on `OK`) with `COLOR_NEON_CYAN` track.
    - **Active Row Highlighting**: Active row renders a 2px left neon indicator, `>` cursor, glowing value/label text, and an illuminated 1px border around its slider track.
    - **Footer Hints**: Single-line guidance at `y = 146` (`OK:Power L/R:Mode`, `L/R:Dim OK:Jump`, `L/R:Red/Green/Blue OK:Toggle`).
    - **Back Navigation**: 1.0s Enter hold steps back from channel mixing to Field 0 (Mode); pressing 1.0s Enter on Field 0 returns to the Launcher Menu; 3.0s Enter hold returns Home.

---

## 7. Companion Web App Architecture (Single Page Application)

Served directly from ESP32-S3 LittleFS / SPIFFS or hosted externally via mDNS (`http://orbit.local`):

### 7.1 Web Application Subsystems
1. **Live HUD Dashboard**: WebSocket-synchronized telemetry cards for Temperature, Humidity, Mic dB, and Wi-Fi RSSI updated at 10Hz.
2. **Dynamic App Remote**: Automatically queries `/api/apps` from the device and generates interactive app launcher cards with active-state highlights.
3. **RGB Studio**: HTML5 color wheel, ambient brightness slider, and preset mode switcher (Breathe, Rainbow, Disco, Pulse, Solid).
4. **Voice Memo & Voice-to-Text Studio**: Live multi-mode voice memo controller (`Voice to Text`, `SD Audio WAV`, `AI Task Extractor`), real-time in-browser speech recognition, automatic speech-to-text transcript generation with one-click clipboard copying (`navigator.clipboard`), TXT export, and full SD Card WAV library management (playback, download, deletion).
5. **Asset Studio**: Client-side canvas utility allowing users to upload any PNG/JPEG image. The browser automatically crops, resizes to 128×160, converts to 16-bit RGB565 BMP, and streams the binary over WebSocket straight into SD card `/images/wallpapers/`.
6. **Notification Transmitter**: Form to push high-priority toast messages to the device screen with custom RGB chime colors.
7. **Firmware OTA & File Upload Studio**:
   - Web-based binary flasher served at `http://orbit.local/update` and embedded inside Settings (`#tab-settings`).
   - Supports flashing compiled firmware (`U_FLASH`) and filesystem images (`U_SPIFFS`) over Wi-Fi with real-time transfer progress, byte tracking, and automatic safe reboot.
   - Rest API (`POST /api/files/upload`) for transferring arbitrary files (wallpapers, web assets, configs, audio) directly to MicroSD card or LittleFS over Wi-Fi.
   - Developer CLI automation scripts (`tools/compile_and_upload.sh` and `tools/upload_file.sh`) to compile via `arduino-cli` / `pio` and deploy over Wi-Fi in a single command.

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

---

## 10. Voice Memo & Multi-Mode Studio Architecture

ORBIT incorporates a complete I2S hardware sampling and multi-mode voice capture pipeline on the ESP32-S3:

### 10.1 Multi-Mode Voice Capture
- **Mode 0: VOICE TO TEXT** — Real-time speech capture streamed to the companion web app for instant transcription and one-click clipboard copying.
- **Mode 1: ASK AI** — Hardware voice question recording. On stop, triggers speech recognition and answers via the Ambient AI engine, syncing the response back to the TFT screen.
- **Mode 2: AUDIO WAV** — High-fidelity 16kHz 16-bit Mono PCM raw recording written to MicroSD card (`/orbit_memos/memo_XXXX.wav`) with SPI bus mutex synchronization.
- **Mode 3: AI TASKS** — Voice note recording parsed automatically for action items, meeting summaries, and task extraction.

### 10.2 Real-time Web Companion Dispatch
- When hardware recording starts or stops, `EventBus` broadcasts `EVENT_RECORDING_START` and `EVENT_RECORDING_STOP`.
- `WebSocketServer` sends `{"type":"memo_event","event":"start|stop","mode":X,"filename":"memo_XXXX.wav"}`.
- Web Companion immediately transcribes the audio using cloud Whisper STT or browser Speech Recognition, populating the hero transcription deck with prominent "Copy to Clipboard" controls.

### 10.3 Web Voice Recording & Audio Player Studio
- **Dual-Source Voice Recorder**: Record directly in the browser via Web Audio / `MediaRecorder` API or trigger ORBIT's hardware I2S microphone over Wi-Fi.
- **Live Visualizers**: Dynamic canvas visualizer animating frequency bars during recording (from `AnalyserNode` or ORBIT live telemetry FFT bands) and during playback.
- **Web Audio Player ("Listen Through Web")**:
  - Full-featured custom player with seekable timeline, scrubber, current/total time display, volume slider, mute, and speed control (0.75x to 2x).
  - 1-click **"🎧 Listen"** button in the SD card recordings library loads any `.wav` file into the web player instantly.
  - Action deck: download audio locally, save browser recordings directly to ORBIT SD card via `POST /api/files/upload`, and push audio to Voice-to-Text transcription.

### 10.4 Firebase Cloud Database & Storage Direct Sync
- **Direct Cloud Upload**: Audio recordings (`.wav` / `.webm`) are streamed directly to Firebase Cloud Storage (`/orbit_recordings/<filename>`).
- **Cloud Metadata Document**: Concurrently inserts a document into Cloud Firestore (or Realtime Database) containing:
  - `filename`, `audio_url`, `duration_sec`, `mode`, `transcript`, `device_id`
  - Real-time sensor telemetry: `temperature_c`, `humidity_pct`, `noise_db`, `recorded_at`
- **Auto-Sync Pipeline**: When "⚡ Auto-Upload to Database Direct" is enabled, both browser-recorded audio and hardware memos from the ESP32-S3 upload immediately to Firebase upon recording stop.
- **Cloud Database Explorer**: Companion dashboard interface to browse cloud recordings, stream audio directly from Firebase Storage URLs, inspect transcripts & telemetry, and manage database records.
- **Endpoints**:
  - `GET /api/firebase/config` — Returns Firebase project, bucket, collection credentials.
  - `POST /api/firebase/sync?file=<name>` — Prepares and validates SD recording for direct Firebase sync.

---

## 11. Ambient Intelligence Studio & On-Device Wi-Fi Screen Sync

ORBIT bridges cloud AI and local hardware telemetry to display intelligent answers directly on the 128x160 TFT display:

### 11.1 AI Engine Options
1. **Google Gemini Flash / Pro**: Cloud generative intelligence via the Google Gemini REST API.
2. **OpenAI GPT-4o / Mini**: Low-latency chat completion via OpenAI REST API.
3. **ORBIT Ambient Assistant**: Zero-dependency local intelligence synthesizing live hardware telemetry (temperature, humidity, noise dB, moon phase, time, and heap).

### 11.2 On-Device AI Answer Overlay
- **Display Geometry**: Word-wraps responses into 18 characters per line (fitting the 112px card width at 6px/char) with up to 8 lines visible per screen window.
- **Navigation & Scrolling**: UP and DOWN buttons scroll line-by-line through long answers; scroll arrows appear when content exceeds the view window.
- **Dismissal**: Pressing [OK] or [BACK] dismisses the overlay and returns to the active carousel app.
- **Dual-Channel Ingestion**: Answers are pushed to the device via WebSocket (`{"cmd":"show_ai_answer","question":"...","answer":"..."}`) or HTTP REST (`POST /api/ai/answer`).
- **REST Endpoints**:
  - `POST /api/ai/answer` — Displays question and answer on the device screen.
  - `GET /api/ai/answer` — Retrieves current active AI overlay state.
  - `POST /api/ai/clear` — Hides the AI answer modal.
