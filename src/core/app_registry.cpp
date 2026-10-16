#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/core/event_bus.h"
#include "src/sensors/mic_sensor.h"
#include "src/display/theme.h"

AppRegistry::~AppRegistry() {
    for (auto* app : _apps) {
        delete app;
    }
    _apps.clear();
}

void AppRegistry::registerApp(AppBase* app) {
    if (app) {
        _apps.push_back(app);
    }
}

void AppRegistry::initAll() {
    for (auto* app : _apps) {
        app->onInit();
    }
    if (!_apps.empty()) {
        _activeIndex = 0;
        _apps[0]->onEnter();
        SharedData::getInstance().setSystemStatus(ESP.getFreeHeap(), millis() / 1000, _apps[0]->getMetadata().id);
    }

    // Subscribe to EventBus to forward to active/background apps
    EventBus::getInstance().subscribe(EVENT_NONE, [this](EventType type, const void* payload) {
        for (size_t i = 0; i < _apps.size(); ++i) {
            if (i == _activeIndex || _apps[i]->getMetadata().backgroundRunning) {
                _apps[i]->onEvent(type, payload);
            }
        }
    });
}

void AppRegistry::update(uint32_t deltaMs) {
    for (size_t i = 0; i < _apps.size(); ++i) {
        if (i == _activeIndex || _apps[i]->getMetadata().backgroundRunning) {
            _apps[i]->onUpdate(deltaMs);
        }
    }
}

void AppRegistry::render(RenderEngine& gfx) {
    TelemetryData telem = SharedData::getInstance().getData();

    // 1. Draw top status bar
    uint8_t currentIcon = (_activeIndex < _apps.size()) ? _apps[_activeIndex]->getMetadata().iconIndex : 0;
    gfx.drawStatusBar(telem.timeStr, telem.wifiConnected, telem.wifiRssi, currentIcon);

    // 2. Render either Carousel or Active App
    if (_inCarousel) {
        renderCarousel(gfx);
    } else {
        if (_activeIndex < _apps.size()) {
            _apps[_activeIndex]->onRender(gfx);
        }
    }

    // 3. Render Voice Recording Banner HUD if active
    if (MicSensor::getInstance().isRecording()) {
        uint32_t durSec = MicSensor::getInstance().getRecordingDurationMs() / 1000;
        gfx.fillRect(20, SCREEN_HEIGHT - 20, SCREEN_WIDTH - 40, 16, COLOR_SLATE_DARK);
        gfx.drawRect(20, SCREEN_HEIGHT - 20, SCREEN_WIDTH - 40, 16, COLOR_NEON_RED);
        gfx.fillCircle(28, SCREEN_HEIGHT - 12, 3, COLOR_NEON_RED);
        char recBuf[16];
        snprintf(recBuf, sizeof(recBuf), "REC %02lu:%02lu", durSec / 60, durSec % 60);
        gfx.drawText(36, SCREEN_HEIGHT - 16, recBuf, COLOR_TEXT_LIGHT, 1);
    }

    // 4. Render Toast notification overlay
    gfx.renderToastOverlay();
}

void AppRegistry::renderCarousel(RenderEngine& gfx) {
    // Fill content background
    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);
    gfx.drawHeader("APP LAUNCHER");

    if (_apps.empty()) return;

    // Carousel Card Center
    int16_t cardX = 14;
    int16_t cardY = STATUS_BAR_HEIGHT + 32;
    int16_t cardW = 100;
    int16_t cardH = 75;

    gfx.drawCard(cardX, cardY, cardW, cardH, COLOR_SLATE_MID, COLOR_NEON_CYAN);

    // Draw active carousel app icon large
    AppMetadata meta = _apps[_carouselIndex]->getMetadata();
    gfx.drawIcon(cardX + 42, cardY + 12, meta.iconIndex, COLOR_NEON_CYAN, COLOR_SLATE_MID);

    // Draw Title
    gfx.drawTextCentered(64, cardY + 36, meta.title, COLOR_TEXT_LIGHT, 1);

    // Draw Indicator Dots / App count
    char idxBuf[16];
    snprintf(idxBuf, sizeof(idxBuf), "%d / %d", (int)_carouselIndex + 1, (int)_apps.size());
    gfx.drawTextCentered(64, cardY + 54, idxBuf, COLOR_TEXT_MUTED, 1);

    // Navigation Hint at bottom
    gfx.drawTextCentered(64, SCREEN_HEIGHT - 12, "< UP / DN >  SELECT", COLOR_TEXT_MUTED, 1);
}

void AppRegistry::handleButtonEvent(const ButtonEvent& evt) {
    // Global PTT Button: Quick Action long-press records voice memo anywhere
    if (evt.button == BTN_ID_ACTION) {
        if (evt.type == BUTTON_CLICK) {
            // Toggle Voice Recording
            if (MicSensor::getInstance().isRecording()) {
                MicSensor::getInstance().stopRecording();
            } else {
                MicSensor::getInstance().startRecording();
            }
            return;
        }
    }

    // Carousel handling
    if (_inCarousel) {
        if (evt.type == BUTTON_CLICK) {
            if (evt.button == BTN_ID_UP) {
                if (_carouselIndex > 0) _carouselIndex--;
                else _carouselIndex = _apps.size() - 1;
            } else if (evt.button == BTN_ID_DOWN) {
                _carouselIndex = (_carouselIndex + 1) % _apps.size();
            } else if (evt.button == BTN_ID_SELECT) {
                switchApp(_carouselIndex);
                _inCarousel = false;
            } else if (evt.button == BTN_ID_BACK) {
                _inCarousel = false;
            }
        }
        return;
    }

    // If inside an app, let the app consume the event first
    if (_activeIndex < _apps.size()) {
        if (_apps[_activeIndex]->onButtonEvent(evt)) {
            return; // App handled it
        }
    }

    // Default global navigation if app didn't consume it
    if (evt.type == BUTTON_CLICK) {
        if (evt.button == BTN_ID_BACK) {
            if (_activeIndex != 0) {
                returnHome(); // Return to Home dashboard
            } else {
                toggleCarousel(); // On Home, BACK opens Carousel
            }
        } else if (evt.button == BTN_ID_SELECT && _activeIndex == 0) {
            toggleCarousel();
        }
    }
}

void AppRegistry::toggleCarousel() {
    _inCarousel = !_inCarousel;
    _carouselIndex = _activeIndex;
}

bool AppRegistry::switchApp(size_t index) {
    if (index >= _apps.size() || index == _activeIndex) {
        return false;
    }

    if (_activeIndex < _apps.size()) {
        _apps[_activeIndex]->onExit();
    }

    _activeIndex = index;
    _apps[_activeIndex]->onEnter();
    SharedData::getInstance().setSystemStatus(ESP.getFreeHeap(), millis() / 1000, _apps[_activeIndex]->getMetadata().id);
    EventBus::getInstance().publish(EVENT_APP_SWITCHED, _apps[_activeIndex]->getMetadata().id);
    return true;
}

bool AppRegistry::switchAppById(const char* id) {
    for (size_t i = 0; i < _apps.size(); ++i) {
        if (strcmp(_apps[i]->getMetadata().id, id) == 0) {
            return switchApp(i);
        }
    }
    return false;
}

void AppRegistry::nextApp() {
    if (_apps.empty()) return;
    switchApp((_activeIndex + 1) % _apps.size());
}

void AppRegistry::prevApp() {
    if (_apps.empty()) return;
    size_t prev = (_activeIndex > 0) ? (_activeIndex - 1) : (_apps.size() - 1);
    switchApp(prev);
}

void AppRegistry::returnHome() {
    switchApp(0);
}

AppBase* AppRegistry::getActiveApp() {
    if (_activeIndex < _apps.size()) {
        return _apps[_activeIndex];
    }
    return nullptr;
}
