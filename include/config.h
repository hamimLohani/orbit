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
#define TFT_BACKLIGHT_MIN_DUTY 20  // Minimum PWM duty cycle at 0% brightness (screen stays visible & readable, never blacked out)

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
#if __has_include("secrets.h")
  #include "secrets.h"
#endif

#ifndef DEFAULT_WIFI_SSID
  #ifdef WIFI_SSID
    #define DEFAULT_WIFI_SSID   WIFI_SSID
  #else
    #define DEFAULT_WIFI_SSID   "ORBIT_WiFi"
  #endif
#endif

#ifndef DEFAULT_WIFI_PASS
  #ifdef WIFI_PASS
    #define DEFAULT_WIFI_PASS   WIFI_PASS
  #else
    #define DEFAULT_WIFI_PASS   ""
  #endif
#endif

// Voice & AI Cloud Secrets Fallbacks
#ifndef DEFAULT_OPENAI_API_KEY
  #ifdef OPENAI_API_KEY
    #define DEFAULT_OPENAI_API_KEY OPENAI_API_KEY
  #else
    #define DEFAULT_OPENAI_API_KEY ""
  #endif
#endif

#ifndef DEFAULT_GEMINI_API_KEY
  #ifdef GEMINI_API_KEY
    #define DEFAULT_GEMINI_API_KEY GEMINI_API_KEY
  #else
    #define DEFAULT_GEMINI_API_KEY ""
  #endif
#endif

#ifndef DEFAULT_ELEVENLABS_API_KEY
  #ifdef ELEVENLABS_API_KEY
    #define DEFAULT_ELEVENLABS_API_KEY ELEVENLABS_API_KEY
  #else
    #define DEFAULT_ELEVENLABS_API_KEY ""
  #endif
#endif

#ifndef DEFAULT_ANTHROPIC_API_KEY
  #ifdef ANTHROPIC_API_KEY
    #define DEFAULT_ANTHROPIC_API_KEY ANTHROPIC_API_KEY
  #else
    #define DEFAULT_ANTHROPIC_API_KEY ""
  #endif
#endif

#ifndef DEFAULT_OPENWEATHER_API_KEY
  #ifdef OPENWEATHER_API_KEY
    #define DEFAULT_OPENWEATHER_API_KEY OPENWEATHER_API_KEY
  #else
    #define DEFAULT_OPENWEATHER_API_KEY ""
  #endif
#endif

#ifndef DEFAULT_WEATHER_CITY
  #define DEFAULT_WEATHER_CITY "London"
#endif

#ifndef ACTIVE_AI_ENGINE
  #ifdef DEFAULT_AI_MODEL
    #define ACTIVE_AI_ENGINE DEFAULT_AI_MODEL
  #else
    #define ACTIVE_AI_ENGINE "gemini"
  #endif
#endif

// Supabase Cloud Database & Storage Fallbacks (100% Free Tier, No Credit Card)
#ifndef DEFAULT_SUPABASE_URL
  #ifdef SUPABASE_URL
    #define DEFAULT_SUPABASE_URL SUPABASE_URL
  #else
    #define DEFAULT_SUPABASE_URL ""
  #endif
#endif

#ifndef DEFAULT_SUPABASE_ANON_KEY
  #ifdef SUPABASE_ANON_KEY
    #define DEFAULT_SUPABASE_ANON_KEY SUPABASE_ANON_KEY
  #else
    #define DEFAULT_SUPABASE_ANON_KEY ""
  #endif
#endif

#ifndef DEFAULT_SUPABASE_STORAGE_BUCKET
  #ifdef SUPABASE_STORAGE_BUCKET
    #define DEFAULT_SUPABASE_STORAGE_BUCKET SUPABASE_STORAGE_BUCKET
  #else
    #define DEFAULT_SUPABASE_STORAGE_BUCKET "orbit-recordings"
  #endif
#endif

#ifndef DEFAULT_SUPABASE_TABLE
  #ifdef SUPABASE_TABLE
    #define DEFAULT_SUPABASE_TABLE SUPABASE_TABLE
  #else
    #define DEFAULT_SUPABASE_TABLE "recordings"
  #endif
#endif

// Firebase Cloud Database & Storage Fallbacks
#ifndef DEFAULT_FIREBASE_PROJECT_ID
  #ifdef FIREBASE_PROJECT_ID
    #define DEFAULT_FIREBASE_PROJECT_ID FIREBASE_PROJECT_ID
  #else
    #define DEFAULT_FIREBASE_PROJECT_ID ""
  #endif
#endif

#ifndef DEFAULT_FIREBASE_API_KEY
  #ifdef FIREBASE_API_KEY
    #define DEFAULT_FIREBASE_API_KEY FIREBASE_API_KEY
  #else
    #define DEFAULT_FIREBASE_API_KEY ""
  #endif
#endif

#ifndef DEFAULT_FIREBASE_STORAGE_BUCKET
  #ifdef FIREBASE_STORAGE_BUCKET
    #define DEFAULT_FIREBASE_STORAGE_BUCKET FIREBASE_STORAGE_BUCKET
  #else
    #define DEFAULT_FIREBASE_STORAGE_BUCKET ""
  #endif
#endif

#ifndef DEFAULT_FIREBASE_COLLECTION
  #ifdef FIREBASE_COLLECTION
    #define DEFAULT_FIREBASE_COLLECTION FIREBASE_COLLECTION
  #else
    #define DEFAULT_FIREBASE_COLLECTION "orbit_recordings"
  #endif
#endif

#ifndef DEFAULT_FIREBASE_RTDB_URL
  #ifdef FIREBASE_RTDB_URL
    #define DEFAULT_FIREBASE_RTDB_URL FIREBASE_RTDB_URL
  #else
    #define DEFAULT_FIREBASE_RTDB_URL ""
  #endif
#endif

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
