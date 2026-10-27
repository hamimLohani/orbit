#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "include/config.h"

// Core Subsystems
#include "src/core/settings_manager.h"
#include "src/core/shared_data.h"
#include "src/core/event_bus.h"
#include "src/core/app_registry.h"

// Hardware Drivers
#include "src/display/render_engine.h"
#include "src/display/bitmap_loader.h"
#include "src/input/button_manager.h"
#include "src/sensors/sensor_manager.h"
#include "src/lighting/rgb_controller.h"
#include "src/lighting/indicator_led.h"

// Network & Web
#include "src/network/wifi_manager.h"
#include "src/network/ntp_sync.h"
#include "src/network/websocket_server.h"
#include "src/network/web_server.h"
#include "src/network/ota_handler.h"

// ==============================================================================
// FreeRTOS Task Handles (Can be suspended/resumed independently)
// ==============================================================================
TaskHandle_t hDisplayTask   = nullptr;
TaskHandle_t hInputTask     = nullptr;
TaskHandle_t hAudioTask     = nullptr;
TaskHandle_t hLightingTask  = nullptr;
TaskHandle_t hSensorTask    = nullptr;
TaskHandle_t hNetworkTask   = nullptr;

// Thread-Safe Button Event Queue (Producer: InputTask, Consumer: DisplayTask)
QueueHandle_t buttonEventQueue = nullptr;

// ==============================================================================
// FreeRTOS Task Implementations
// ==============================================================================

// ------------------------------------------------------------------------------
// 1. AudioTask (Core 0, High Priority: 4)
// Dedicated continuous I2S DMA audio sampling, 16-band FFT, & WAV recording
// ------------------------------------------------------------------------------
void taskAudio(void* pvParameters) {
    while (true) {
        SensorManager::getInstance().updateAudio();
        // Yield 5ms to allow other tasks on Core 0 to run
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ------------------------------------------------------------------------------
// 2. NetworkTask (Core 0, Low Priority: 1)
// WiFi radio stack, NTP clock sync, WebSockets, WebServer, OTA updates
// ------------------------------------------------------------------------------
void taskNetwork(void* pvParameters) {
    while (true) {
        WifiManager::getInstance().update();
        NtpSync::getInstance().update();
        WebSocketServer::getInstance().update();
        OtaHandler::getInstance().update();
        // Low-latency yield keeping ESP-IDF WiFi watchdog happy
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ------------------------------------------------------------------------------
// 3. InputTask (Core 1, High Priority: 4)
// High-frequency responsive button debounce and state tracking
// ------------------------------------------------------------------------------
void taskInput(void* pvParameters) {
    while (true) {
        ButtonManager::getInstance().update();
        // 10ms sampling tick guarantees crisp responsiveness with zero busy waiting
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ------------------------------------------------------------------------------
// 4. DisplayTask (Core 1, Medium Priority: 3)
// UI event consumption, App updates, offscreen rendering, and ST7735 SPI flush
// ------------------------------------------------------------------------------
void taskDisplay(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint32_t lastFrameMs = millis();

    while (true) {
        // 1. Drain asynchronous button events from InputTask
        ButtonEvent evt;
        while (buttonEventQueue && xQueueReceive(buttonEventQueue, &evt, 0) == pdTRUE) {
            AppRegistry::getInstance().handleButtonEvent(evt);
        }

        uint32_t now = millis();
        uint32_t deltaMs = now - lastFrameMs;
        lastFrameMs = now;

        // 2. Update active application and background services
        AppRegistry::getInstance().update(deltaMs);

        // 3. Render graphics to double-buffered canvas in RAM
        AppRegistry::getInstance().render(RenderEngine::getInstance());

        // 4. Flush frame to ST7735 over SPI (guarded by _spiMutex)
        RenderEngine::getInstance().flush();

        // 5. Deterministic frame pacing (~30 FPS)
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(FRAME_INTERVAL_MS));
    }
}

// ------------------------------------------------------------------------------
// 5. LightingTask (Core 1, Medium-Low Priority: 2)
// WS2812 addressable RGB animations and indicator alert LED patterns
// ------------------------------------------------------------------------------
void taskLighting(void* pvParameters) {
    while (true) {
        RgbController::getInstance().update();
        IndicatorLed::getInstance().update();
        // 50Hz LED animation rate
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ------------------------------------------------------------------------------
// 6. SensorTask (Core 1, Medium-Low Priority: 2)
// Environmental telemetry polling (DHT11 temperature & humidity)
// ------------------------------------------------------------------------------
void taskSensor(void* pvParameters) {
    while (true) {
        SensorManager::getInstance().updateClimate();
        // Poll every 250ms (DHT11 internal gate enforces 2.5s safe intervals)
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

// ==============================================================================
// Arduino Entrypoint
// ==============================================================================

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.printf("\n==========================================\n");
    Serial.printf("   ORBIT v%s Embedded OS Starting...\n", ORBIT_VERSION_STR);
    Serial.printf("==========================================\n");

    // 1. Core Services
    SettingsManager::getInstance().init();
    SharedData::getInstance().init();

    // 2. Display & Storage (SPI Shared Bus)
    RenderEngine::getInstance().init();
    BitmapLoader::getInstance().initSdCard();

    // Show Boot Splash
    RenderEngine::getInstance().fillScreen(COLOR_VOID_DARK);
    RenderEngine::getInstance().drawCircle(64, 70, 24, COLOR_NEON_CYAN);
    RenderEngine::getInstance().drawCircle(64, 70, 16, COLOR_NEON_PURPLE);
    RenderEngine::getInstance().drawTextCentered(64, 66, "ORBIT", COLOR_TEXT_LIGHT, 2);
    RenderEngine::getInstance().drawTextCentered(64, 105, "INITIALIZING OS", COLOR_TEXT_MUTED, 1);

    // 3. User Input & Thread-Safe Event Queue
    buttonEventQueue = xQueueCreate(16, sizeof(ButtonEvent));
    ButtonManager::getInstance().init();
    ButtonManager::getInstance().setCallback([](const ButtonEvent& evt) {
        if (buttonEventQueue) {
            xQueueSend(buttonEventQueue, &evt, 0);
        }
    });

    // 4. Lighting & Indicators
    RgbController::getInstance().init();
    IndicatorLed::getInstance().init();

    // 5. Environmental Sensors & I2S Audio
    SensorManager::getInstance().init();

    // 6. Network & Web Services
    WifiManager::getInstance().init();
    NtpSync::getInstance().init();
    WebServer::getInstance().init();
    OtaHandler::getInstance().init();

    // 7. Initialize All Registered Apps
    AppRegistry::getInstance().initAll();

    // 8. Spawn FreeRTOS Tasks with Explicit Scheduling Priorities & Core Pinning

    // Core 0: High-Priority Audio DSP Sampling
    xTaskCreatePinnedToCore(
        taskAudio,
        "AudioTask",
        4096,
        NULL,
        4, // High Priority
        &hAudioTask,
        0  // Core 0
    );

    // Core 0: Low-Priority Background Networking & Web
    xTaskCreatePinnedToCore(
        taskNetwork,
        "NetworkTask",
        8192,
        NULL,
        1, // Low Priority
        &hNetworkTask,
        0  // Core 0
    );

    // Core 1: High-Priority Input Debouncing
    xTaskCreatePinnedToCore(
        taskInput,
        "InputTask",
        3072,
        NULL,
        4, // High Priority
        &hInputTask,
        1  // Core 1
    );

    // Core 1: Medium-Priority Display Rendering & UI
    xTaskCreatePinnedToCore(
        taskDisplay,
        "DisplayTask",
        8192,
        NULL,
        3, // Medium Priority
        &hDisplayTask,
        1  // Core 1
    );

    // Core 1: Lighting & Visual Feedback
    xTaskCreatePinnedToCore(
        taskLighting,
        "LightingTask",
        3072,
        NULL,
        2, // Medium-Low Priority
        &hLightingTask,
        1  // Core 1
    );

    // Core 1: Environmental Sensor Polling
    xTaskCreatePinnedToCore(
        taskSensor,
        "SensorTask",
        3072,
        NULL,
        2, // Medium-Low Priority
        &hSensorTask,
        1  // Core 1
    );

    // Initial light state: Off by default
    IndicatorLed::getInstance().setPattern(LED_OFF);
    RgbController::getInstance().setMode(RGB_OFF);

    Serial.println("[ORBIT] FreeRTOS Multi-Task OS Initialized Successfully.");
}

void loop() {
    // Arduino loopTask acts as a system monitor / watchdog
    vTaskDelay(pdMS_TO_TICKS(2000));
}
