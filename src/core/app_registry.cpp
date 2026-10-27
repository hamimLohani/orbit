#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/core/event_bus.h"
#include "src/sensors/mic_sensor.h"
#include "src/display/theme.h"

AppRegistry::AppRegistry() {
    _registryMutex = xSemaphoreCreateRecursiveMutex();
}

AppRegistry::~AppRegistry() {
    for (auto* app : _apps) {
        delete app;
    }
    _apps.clear();
    if (_registryMutex) {
        vSemaphoreDelete(_registryMutex);
        _registryMutex = nullptr;
    }
}

void AppRegistry::registerApp(AppBase* app) {
    if (app) {
        _apps.push_back(app);
    }
}

void AppRegistry::initAll() {
    // Ensure "home" dashboard app is always at index 0 regardless of link order
    for (size_t i = 0; i < _apps.size(); ++i) {
        if (strcmp(_apps[i]->getMetadata().id, "home") == 0) {
            if (i != 0) {
                AppBase* home = _apps[i];
                _apps.erase(_apps.begin() + i);
                _apps.insert(_apps.begin(), home);
            }
            break;
        }
    }

    for (auto* app : _apps) {
        app->onInit();
    }
    if (!_apps.empty()) {
        _activeIndex = 0;
        _inCarousel = false;
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
    if (_registryMutex && xSemaphoreTakeRecursive(_registryMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        for (size_t i = 0; i < _apps.size(); ++i) {
            if (i == _activeIndex || _apps[i]->getMetadata().backgroundRunning) {
                _apps[i]->onUpdate(deltaMs);
            }
        }
        xSemaphoreGiveRecursive(_registryMutex);
    }
}

void AppRegistry::render(RenderEngine& gfx) {
    if (_registryMutex && xSemaphoreTakeRecursive(_registryMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
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

        xSemaphoreGiveRecursive(_registryMutex);
    }
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
    gfx.drawTextCentered(64, SCREEN_HEIGHT - 12, "< L / R >  ENTER: Select", COLOR_TEXT_MUTED, 1);
}

void AppRegistry::handleButtonEvent(const ButtonEvent& evt) {
    if (_registryMutex && xSemaphoreTakeRecursive(_registryMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        // 1. Threshold: 2.5s Long Hold on ENTER -> Direct return to HOME
        if (evt.button == BTN_ID_ENTER && evt.type == BUTTON_HOLD_2S) {
            returnHome();
            _inCarousel = false;
            RenderEngine::getInstance().showToast("HOME", "Returned to Home", COLOR_NEON_CYAN, 1200);
            xSemaphoreGiveRecursive(_registryMutex);
            return;
        }

        // 2. Threshold: 1.0s Hold on ENTER -> Go BACK ("1s enter=back")
        if (evt.button == BTN_ID_ENTER && (evt.type == BUTTON_HOLD_1S || evt.type == BUTTON_HOLD_05S)) {
            if (_inCarousel) {
                _inCarousel = false;
                RenderEngine::getInstance().showToast("BACK", "Closed Launcher", COLOR_TEXT_MUTED, 800);
            } else if (_activeIndex != 0) {
                returnHome();
                RenderEngine::getInstance().showToast("BACK", "Returned to Home", COLOR_TEXT_MUTED, 1000);
            }
            xSemaphoreGiveRecursive(_registryMutex);
            return;
        }

        // 3. Carousel Mode Navigation
        if (_inCarousel) {
            if (evt.type == BUTTON_CLICK || evt.type == BUTTON_REPEAT) {
                if (evt.button == BTN_ID_UP || evt.button == BTN_ID_LEFT) {
                    if (_carouselIndex > 0) _carouselIndex--;
                    else _carouselIndex = _apps.size() - 1;
                } else if (evt.button == BTN_ID_DOWN || evt.button == BTN_ID_RIGHT) {
                    _carouselIndex = (_carouselIndex + 1) % _apps.size();
                } else if (evt.button == BTN_ID_ENTER) {
                    switchApp(_carouselIndex);
                    _inCarousel = false;
                }
            }
            xSemaphoreGiveRecursive(_registryMutex);
            return;
        }

        // 4. Active App Input Delegation
        if (_activeIndex < _apps.size()) {
            if (_apps[_activeIndex]->onButtonEvent(evt)) {
                xSemaphoreGiveRecursive(_registryMutex);
                return; // App consumed the event
            }
        }

        // 5. Default Navigation on Home Dashboard
        if (evt.type == BUTTON_CLICK) {
            if (evt.button == BTN_ID_ENTER) {
                toggleCarousel(); // Short tap on Enter opens app launcher
            } else if (evt.button == BTN_ID_LEFT || evt.button == BTN_ID_UP) {
                prevApp();
            } else if (evt.button == BTN_ID_RIGHT || evt.button == BTN_ID_DOWN) {
                nextApp();
            }
        }

        xSemaphoreGiveRecursive(_registryMutex);
    }
}

void AppRegistry::toggleCarousel() {
    _inCarousel = !_inCarousel;
    _carouselIndex = _activeIndex;
}

bool AppRegistry::switchApp(size_t index) {
    if (_registryMutex && xSemaphoreTakeRecursive(_registryMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        if (index >= _apps.size() || index == _activeIndex) {
            xSemaphoreGiveRecursive(_registryMutex);
            return false;
        }

        if (_activeIndex < _apps.size()) {
            _apps[_activeIndex]->onExit();
        }

        _activeIndex = index;
        _apps[_activeIndex]->onEnter();
        SharedData::getInstance().setSystemStatus(ESP.getFreeHeap(), millis() / 1000, _apps[_activeIndex]->getMetadata().id);
        xSemaphoreGiveRecursive(_registryMutex);

        EventBus::getInstance().publish(EVENT_APP_SWITCHED, _apps[_activeIndex]->getMetadata().id);
        return true;
    }
    return false;
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
    _inCarousel = false;
    switchApp(0);
}

AppBase* AppRegistry::getActiveApp() {
    if (_activeIndex < _apps.size()) {
        return _apps[_activeIndex];
    }
    return nullptr;
}
