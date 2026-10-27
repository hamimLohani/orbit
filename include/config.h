#pragma once
#include <Arduino.h>

// ==============================================================================
// ORBIT v2 — Central Configuration & Pinout Definition
// Target: ESP32-S3 (Dual Core LX7, 8MB Flash, PSRAM)
// ==============================================================================

#define ORBIT_VERSION_MAJOR 2
#define ORBIT_VERSION_MINOR 0
#define ORBIT_VERSION_PATCH 0
#define ORBIT_VERSION_STR   "2.0.0"

// ------------------------------------------------------------------------------
// 1. SPI Bus Hardware Pins (Shared between ST7735 and MicroSD)
// ------------------------------------------------------------------------------
#define PIN_SPI_MOSI          11
#define PIN_SPI_MISO          13
#define PIN_SPI_SCK           12

// ST7735 1.8" TFT (128x160 RGB565)
#define PIN_TFT_CS            10
#define PIN_TFT_DC            9
#define PIN_TFT_RST           14
#define PIN_TFT_BL            21   // Backlight PWM pin

// MicroSD Card (Dedicated CS)
#define PIN_SD_CS             47

// Display Geometry
#define SCREEN_WIDTH          128
#define SCREEN_HEIGHT         160
#define STATUS_BAR_HEIGHT     14
#define CONTENT_HEIGHT        (SCREEN_HEIGHT - STATUS_BAR_HEIGHT)

// ------------------------------------------------------------------------------
// 2. I2S Digital MEMS Microphone (INMP441)
// ------------------------------------------------------------------------------
#define PIN_I2S_SCK           5    // Serial Clock (BCLK)
#define PIN_I2S_WS            4    // Word Select (LRCLK)
#define PIN_I2S_SD            6    // Serial Data Out (DOUT)

#define I2S_SAMPLE_RATE       16000
#define I2S_BUFFER_SIZE       512
#define FFT_NUM_BINS          16   // 16 Frequency Bands for Visualizer

// ------------------------------------------------------------------------------
// 3. Environmental Sensors & Lighting
// ------------------------------------------------------------------------------
#define PIN_DHT11             7    // DHT11 Data Pin
#define DHT_TYPE              11   // DHT 11

#define PIN_RGB_LED           48   // Built-in WS2812 RGB LED
#define PIN_INDICATOR_LED     2    // High-visibility alert LED

// ------------------------------------------------------------------------------
// 4. Navigation Buttons (Active LOW with internal pull-up)
// ------------------------------------------------------------------------------
#define PIN_BTN_UP            1
#define PIN_BTN_DOWN          3
#define PIN_BTN_LEFT          17   // Left navigation (was Back)
#define PIN_BTN_RIGHT         18   // Right navigation (was Select)
#define PIN_BTN_ENTER         16   // Enter / Action (.5s=Back, 2s=Home)

// Backward compatibility aliases
#define PIN_BTN_SELECT        PIN_BTN_RIGHT
#define PIN_BTN_BACK          PIN_BTN_LEFT
#define PIN_BTN_ACTION        PIN_BTN_ENTER

// Button Debounce & Hold Timings (ms)
#define BTN_DEBOUNCE_MS       25
#define BTN_HOLD_1S_MS        1000 // 1.0s -> 1 step back ("1s enter = 1 step back")
#define BTN_HOLD_3S_MS        3000 // 3.0s -> Home ("3s enter = home")
#define BTN_HOLD_2S_MS        BTN_HOLD_3S_MS // Backward compatibility alias
#define BTN_HOLD_05S_MS       BTN_HOLD_1S_MS // Backward compatibility alias
#define BTN_REPEAT_DELAY_MS   350  // Initial hold delay before repeat
#define BTN_REPEAT_MS         120  // Continuous repeat interval

// ------------------------------------------------------------------------------
// 5. System Timings & Schedulers (ms)
// ------------------------------------------------------------------------------
#define UI_TARGET_FPS         30
#define FRAME_INTERVAL_MS     (1000 / UI_TARGET_FPS)

#define SENSOR_POLL_DHT_MS    2500 // DHT11 minimum safe read interval
#define SENSOR_POLL_MIC_MS    40   // Mic level & FFT refresh (25Hz)
#define TELEMETRY_BROADCAST_MS 100 // WebSocket telemetry broadcast (10Hz)
#define NTP_SYNC_INTERVAL_MS  (3600 * 1000) // 1 Hour

// ------------------------------------------------------------------------------
// 6. Network & Web Services
// ------------------------------------------------------------------------------
#define DEFAULT_WIFI_SSID     "ORBIT_WiFi"
#define DEFAULT_WIFI_PASS     ""
#define DEFAULT_AP_SSID       "ORBIT-Setup"
#define DEFAULT_AP_PASS       "orbit1234"
#define MDNS_HOSTNAME         "orbit"
#define HTTP_PORT             80
#define WS_PORT               80

// ------------------------------------------------------------------------------
// 7. Storage Paths (MicroSD Card)
// ------------------------------------------------------------------------------
#define SD_PATH_WALLPAPERS    "/images/wallpapers"
#define SD_PATH_ICONS         "/images/icons"
#define SD_PATH_MEMOS         "/recordings/memos"
#define SD_PATH_LOGS          "/system/logs"
