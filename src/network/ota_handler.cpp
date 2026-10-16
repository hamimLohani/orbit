#include "src/network/ota_handler.h"
#include "include/config.h"

void OtaHandler::init() {
    ArduinoOTA.setHostname(MDNS_HOSTNAME);
    
    ArduinoOTA.onStart([]() {
        // Prepare for OTA
    });

    ArduinoOTA.onEnd([]() {
        // Flash complete
    });

    ArduinoOTA.onError([](ota_error_t error) {
        // Error handling
    });

    ArduinoOTA.begin();
    _initialized = true;
}

void OtaHandler::update() {
    if (_initialized) {
        ArduinoOTA.handle();
    }
}
