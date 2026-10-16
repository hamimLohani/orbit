#include <Arduino.h>
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

// Background Network & Audio Task on Core 0
void networkAndAudioTask(void* pvParameters) {
    while (true) {
        // High-frequency sensor & network polling
        SensorManager::getInstance().updateSensors();
        WifiManager::getInstance().update();
        NtpSync::getInstance().update();
        WebSocketServer::getInstance().update();
        OtaHandler::getInstance().update();

        // 5ms delay yield to keep watchdog happy
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

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

    // 3. User Input
    ButtonManager::getInstance().init();
    ButtonManager::getInstance().setCallback([](const ButtonEvent& evt) {
        AppRegistry::getInstance().handleButtonEvent(evt);
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

    // 8. Spawn Core 0 Background Task for Network & Audio DMA
    xTaskCreatePinnedToCore(
        networkAndAudioTask,
        "NetAudioTask",
        8192,
        NULL,
        1,
        NULL,
        0 // Core 0
    );

    // Initial light state: Off by default
    IndicatorLed::getInstance().setPattern(LED_OFF);
    RgbController::getInstance().setMode(RGB_OFF);

    Serial.println("[ORBIT] OS Initialized Successfully.");
}

void loop() {
    static uint32_t lastFrameTime = millis();
    uint32_t now = millis();
    uint32_t deltaMs = now - lastFrameTime;
    if (deltaMs < FRAME_INTERVAL_MS) {
        delay(1);
        return;
    }
    lastFrameTime = now;

    // 1. Process Debounced Button Inputs
    ButtonManager::getInstance().update();

    // 2. Update Active App & Background Services
    AppRegistry::getInstance().update(deltaMs);

    // 3. Render Graphics Frame to off-screen buffer
    AppRegistry::getInstance().render(RenderEngine::getInstance());

    // 4. Flush complete frame to ST7735 in a single SPI burst (zero flicker!)
    RenderEngine::getInstance().flush();

    // 5. Update RGB Lighting & Indicator LED animations
    RgbController::getInstance().update();
    IndicatorLed::getInstance().update();
}
