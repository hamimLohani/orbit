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

        // 5. Render AI Answer modal overlay if active (while connected to Wi-Fi)
        gfx.renderAiAnswerOverlay();

        xSemaphoreGiveRecursive(_registryMutex);
    }
}

void AppRegistry::renderCarousel(RenderEngine& gfx) {
    // Fill content background
    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);
    gfx.drawHeader("APP LAUNCHER");

    if (_apps.empty()) return;

    // Carousel Card Center (y=40..108)
    int16_t cardX = 14;
    int16_t cardY = STATUS_BAR_HEIGHT + 26; // 14 + 26 = 40
    int16_t cardW = 100;
    int16_t cardH = 68; // ends cleanly at 108

    gfx.drawCard(cardX, cardY, cardW, cardH, COLOR_SLATE_MID, COLOR_NEON_CYAN);

    // Draw active carousel app icon
    AppMetadata meta = _apps[_carouselIndex]->getMetadata();
    gfx.drawIcon(cardX + 42, cardY + 10, meta.iconIndex, COLOR_NEON_CYAN, COLOR_SLATE_MID);

    // Draw Title
    gfx.drawTextCentered(64, cardY + 31, meta.title, COLOR_TEXT_LIGHT, 1);

    // Draw App index badge
    char idxBuf[16];
    snprintf(idxBuf, sizeof(idxBuf), "APP %d / %d", (int)_carouselIndex + 1, (int)_apps.size());
    gfx.drawTextCentered(64, cardY + 49, idxBuf, COLOR_NEON_AMBER, 1);

    // Carousel App Tracker Dots at y=114 (pill for active app)
    int totalApps = (int)_apps.size();
    if (totalApps > 1) {
        int dotSpacing = 7;
        int totalWidth = (totalApps - 1) * dotSpacing;
        int startX = 64 - (totalWidth / 2);
        for (int i = 0; i < totalApps; ++i) {
            int dx = startX + (i * dotSpacing);
            if (i == (int)_carouselIndex) {
                gfx.fillRect(dx - 2, 114, 5, 3, COLOR_NEON_CYAN); // Active pill
            } else {
                gfx.fillRect(dx, 115, 2, 2, COLOR_SLATE_BORDER); // Inactive dot
            }
        }
    }

    // Divider line at y=123
    gfx.drawFastHLine(14, 123, 100, COLOR_SLATE_BORDER);

    // Non-overlapping Menu Footer Navigation (19 chars = 114px, centered with 7px margins on each side)
    gfx.drawTextCentered(64, 129, "< L / R >  OK: Open", COLOR_NEON_CYAN, 1);

    if (!MicSensor::getInstance().isRecording()) {
        gfx.drawTextCentered(64, 143, "1s: Back   3s: Home", COLOR_TEXT_MUTED, 1);
    }
}

void AppRegistry::handleButtonEvent(const ButtonEvent& evt) {
    if (_registryMutex && xSemaphoreTakeRecursive(_registryMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        // 0. AI Answer Modal Overlay Interception (Scroll / Dismiss)
        if (RenderEngine::getInstance().isAiAnswerActive()) {
            if (evt.type == BUTTON_CLICK || evt.type == BUTTON_REPEAT) {
                if (evt.button == BTN_ID_UP) {
                    RenderEngine::getInstance().scrollAiAnswer(-1);
                    xSemaphoreGiveRecursive(_registryMutex);
                    return;
                } else if (evt.button == BTN_ID_DOWN) {
                    RenderEngine::getInstance().scrollAiAnswer(+1);
                    xSemaphoreGiveRecursive(_registryMutex);
                    return;
                } else if (evt.button == BTN_ID_ENTER || evt.button == BTN_ID_SELECT || evt.button == BTN_ID_LEFT || evt.button == BTN_ID_BACK) {
                    RenderEngine::getInstance().hideAiAnswer();
                    xSemaphoreGiveRecursive(_registryMutex);
                    return;
                }
            }
            xSemaphoreGiveRecursive(_registryMutex);
            return;
        }

        // 1. Threshold: 3.0s Long Hold on ENTER -> Direct return to HOME ("3s enter = home")
        if (evt.button == BTN_ID_ENTER && (evt.type == BUTTON_HOLD_3S || evt.type == BUTTON_HOLD_2S)) {
            returnHome();
            _inCarousel = false;
            RenderEngine::getInstance().showToast("HOME", "Returned to Home", COLOR_NEON_CYAN, 1200);
            xSemaphoreGiveRecursive(_registryMutex);
            return;
        }

        // 2. Threshold: 1.0s Hold on ENTER -> 1 Step Back ("1s enter = 1 step back")
        if (evt.button == BTN_ID_ENTER && (evt.type == BUTTON_HOLD_1S || evt.type == BUTTON_HOLD_05S)) {
            // A. If Carousel is open, 1 step back closes the launcher
            if (_inCarousel) {
                _inCarousel = false;
                RenderEngine::getInstance().showToast("BACK", "Closed Launcher", COLOR_TEXT_MUTED, 800);
                xSemaphoreGiveRecursive(_registryMutex);
                return;
            }

            // B. Give active app a chance to step back internally (e.g. in-game -> menu, subpage -> page 0)
            if (_activeIndex < _apps.size()) {
                if (_apps[_activeIndex]->onButtonEvent(evt)) {
                    xSemaphoreGiveRecursive(_registryMutex);
                    return; // Active app consumed 1 step back
                }
            }

            // C. If active app did not consume it, step back to the Launcher Menu
            if (_activeIndex != 0) {
                _inCarousel = true;
                _carouselIndex = _activeIndex;
                RenderEngine::getInstance().showToast("MENU", "App Launcher", COLOR_TEXT_MUTED, 800);
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

        // 4. Active App Input Delegation (app handles its own internal controls)
        if (_activeIndex < _apps.size()) {
            if (_apps[_activeIndex]->onButtonEvent(evt)) {
                xSemaphoreGiveRecursive(_registryMutex);
                return; // App consumed the event
            }
        }

        // 5. Default Navigation strictly on Home Dashboard (app 0)
        // Inside any app, unhandled L/R does NOT change apps; user must hold 1s Enter to back out to menu first
        if (_activeIndex == 0 && evt.type == BUTTON_CLICK) {
            if (evt.button == BTN_ID_ENTER) {
                toggleCarousel(); // Short tap on Enter opens app launcher
            } else if (evt.button == BTN_ID_LEFT || evt.button == BTN_ID_UP) {
                _inCarousel = true;
                if (_carouselIndex > 0) _carouselIndex--;
                else _carouselIndex = _apps.size() - 1;
            } else if (evt.button == BTN_ID_RIGHT || evt.button == BTN_ID_DOWN) {
                _inCarousel = true;
                _carouselIndex = (_carouselIndex + 1) % _apps.size();
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
            _previousIndex = _activeIndex;
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
