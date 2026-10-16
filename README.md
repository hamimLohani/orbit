# ORBIT v2 — Ambient Sensing & Companion Embedded OS
**Production Firmware for ESP32-S3 | Card-Based OS Architecture**

[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-orange.svg)](https://www.espressif.com/)
[![Framework](https://img.shields.io/badge/Framework-Arduino%20%2F%20PlatformIO-blue.svg)](https://platformio.org/)
[![Display](https://img.shields.io/badge/Display-ST7735%201.8%22%20128x160-purple.svg)]()
[![License](https://img.shields.io/badge/License-MIT-green.svg)]()

ORBIT is an ambient sensing companion and desktop embedded operating system running on the **ESP32-S3**. It features an extensible polymorphic app framework, real-time 16-band DSP audio analysis, non-blocking telemetry, an interactive virtual companion with emotional reactivity, and a bi-directional WebSocket companion web dashboard.

---

## 🌟 What's New in v2.0

- 👾 **Aura Companion & Emotion Engine**: Dynamic procedural animated pixel eyes that react to room temperature, noise spikes, music rhythm, and sleep cycles.
- 📊 **Real-Time 16-Band DSP Audio Spectrum**: Continuous INMP441 I2S DMA audio sampling with hardware-accelerated FFT frequency binning, peak-hold decay, phosphor VU meters, and oscilloscope mode.
- ⏱️ **Focus Flow (Smart Pomodoro + Noise Sentinel)**: Circular orbit countdown timer with an ambient background noise watchdog that alerts you if room noise exceeds focus thresholds.
- 🎙️ **Voice Memo Recorder & AI Pipeline**: One-touch PTT voice capture directly to SD card as 16kHz WAV files (`/recordings/memos/memo_xxx.wav`) with browser audio playback and Whisper AI transcription integration.
- 🪐 **Space & Astro Dashboard**: Procedural lunar phase calculator with illumination percentage and ISS International Space Station orbit pass tracker.
- 🚀 **Orbit Lander Retro Mini-Game**: Vector physics lunar lander arcade game playable via the 5 navigation buttons.
- 🔔 **Global Toast Notification Overlay**: Sliding top-banner alert system that can display priority webhook messages across any active app.
- 🧩 **Extensible Polymorphic App SDK**: Add any new application to the OS in under 5 minutes with zero modifications to core OS files using `AppBase` and `REGISTER_APP()`.

---

## 🔌 Hardware Pinout & Wiring

> [!TIP]
> **Complete Wiring Schematics & Breadboard Diagrams**: See the full [Hardware Connection Guide](docs/HARDWARE_CONNECTION.md) for module-by-module circuit diagrams, wire color coding, and troubleshooting tips.

All pin assignments are centralized in [`include/config.h`](include/config.h):

| Subsystem | Signal | ESP32-S3 GPIO | Notes |
| :--- | :--- | :--- | :--- |
| **ST7735 TFT** | MOSI / SDA | GPIO 11 | Shared high-speed SPI bus |
| | MISO | GPIO 13 | Shared high-speed SPI bus |
| | SCK / SCL | GPIO 12 | Shared high-speed SPI bus |
| | CS | GPIO 10 | Dedicated Display Chip Select |
| | DC / A0 | GPIO 9 | Data / Command |
| | RESET | GPIO 14 | Hardware Reset |
| | BL / LED | GPIO 21 | PWM Backlight Dimming |
| **MicroSD Card** | SD_CS | GPIO 47 | Dedicated SD Chip Select (Mutex coordinated) |
| **INMP441 Mic** | SCK / BCLK | GPIO 5 | I2S Bit Clock |
| | WS / LRCLK | GPIO 4 | I2S Word Select |
| | SD / DOUT | GPIO 6 | I2S Data from mic |
| **Sensors** | DHT11 Data | GPIO 7 | Single-wire with 10k pullup |
| **Lighting** | WS2812 RGB | GPIO 48 | Built-in ESP32-S3 Addressable RGB |
| | Indicator LED | GPIO 2 | Dedicated Alert Beacon LED |
| **Buttons** | BTN_UP | GPIO 1 | Directional UP / Thrust (Active LOW to GND) |
| | BTN_DOWN | GPIO 3 | Directional DOWN / Drift reverse (Active LOW to GND) |
| | BTN_LEFT | GPIO 17 | Directional LEFT / Steer left / Dim (Active LOW to GND) |
| | BTN_RIGHT | GPIO 18 | Directional RIGHT / Steer right / Brighten (Active LOW to GND) |
| | BTN_ENTER | GPIO 16 | Tap: Enter | Hold 0.5s: Back | Hold 2.0s: Home |

---

## 🚀 Building & Flashing

### Option A: Using Arduino CLI
```bash
# Compile
arduino-cli compile --fqbn esp32:esp32:esp32s3 .

# Flash to device
arduino-cli upload -p /dev/cu.usbmodem* --fqbn esp32:esp32:esp32s3 .
```

### Option B: Using PlatformIO
```bash
# Build firmware
pio run

# Upload firmware
pio run --target upload

# Upload web assets to LittleFS
pio run --target uploadfs

# Open Serial Monitor
pio device monitor
```

---

## 📱 Web Companion Dashboard

Once booted, ORBIT starts an mDNS responder:
- Open your browser to: **`http://orbit.local`** (or the IP displayed on the Network screen).
- **Fallback AP Mode**: If no Wi-Fi credentials are saved, connect your phone or laptop to Wi-Fi SSID **`ORBIT-Setup`** (password: `orbit1234`) and browse to `http://192.168.4.1`.

---

## 🧩 Adding New Apps in Future (5-Minute Guide)

See [`docs/ADDING_NEW_APPS.md`](docs/ADDING_NEW_APPS.md) for full instructions:
1. Copy [`src/apps/app_template/`](src/apps/app_template/) to `src/apps/my_new_app/`.
2. Implement your UI rendering in `onRender(RenderEngine& gfx)`.
3. Add `REGISTER_APP(MyNewApp);` at the top of your `.cpp` file.
4. Compile — your new app will automatically appear in the carousel and on the web dashboard!
