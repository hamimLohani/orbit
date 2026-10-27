#include "src/network/ota_handler.h"
#include "src/display/render_engine.h"
#include "include/config.h"

void OtaHandler::init() {
    ArduinoOTA.setHostname(MDNS_HOSTNAME);
    
    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "Firmware" : "Filesystem";
        Serial.printf("[OTA] Starting %s update...\n", type.c_str());
        OtaHandler::getInstance().setUpdating(true, 0, "Flashing...");
        RenderEngine::getInstance().showToast("OTA UPDATE", "Receiving firmware...", COLOR_NEON_CYAN, 5000);
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        uint8_t pct = (total > 0) ? (uint8_t)((progress * 100) / total) : 0;
        OtaHandler::getInstance().setUpdating(true, pct, "Flashing...");
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Update Complete! Rebooting...");
        OtaHandler::getInstance().setUpdating(false, 100, "Done! Rebooting");
        RenderEngine::getInstance().showToast("OTA SUCCESS", "Rebooting system...", COLOR_NEON_MINT, 3000);
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]\n", error);
        OtaHandler::getInstance().setUpdating(false, 0, "Error");
        RenderEngine::getInstance().showToast("OTA ERROR", "Update failed!", COLOR_NEON_RED, 4000);
    });

    ArduinoOTA.begin();
    _initialized = true;
    Serial.println("[OTA] ArduinoOTA listening on port 3232.");
}

void OtaHandler::setUpdating(bool updating, uint8_t percent, const char* status) {
    _isUpdating = updating;
    _progressPercent = percent;
    if (status && status[0]) {
        strncpy(_statusText, status, sizeof(_statusText) - 1);
        _statusText[sizeof(_statusText) - 1] = 0;
    }
}

void OtaHandler::update() {
    if (_initialized) {
        ArduinoOTA.handle();
    }
}
