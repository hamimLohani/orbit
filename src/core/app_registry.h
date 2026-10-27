#pragma once
#include <Arduino.h>
#include <vector>
#include "src/core/app_base.h"

// ==============================================================================
// ORBIT v2 — Dynamic App Registry & Carousel Navigation Manager
// ==============================================================================

class AppRegistry {
public:
    static AppRegistry& getInstance() {
        static AppRegistry instance;
        return instance;
    }

    void registerApp(AppBase* app);
    void initAll();

    void update(uint32_t deltaMs);
    void render(RenderEngine& gfx);
    void handleButtonEvent(const ButtonEvent& evt);

    bool switchApp(size_t index);
    bool switchAppById(const char* id);
    void nextApp();
    void prevApp();
    void returnHome();

    AppBase* getActiveApp();
    size_t getActiveIndex() const { return _activeIndex; }
    size_t getAppCount() const { return _apps.size(); }
    const std::vector<AppBase*>& getApps() const { return _apps; }

    bool isInCarousel() const { return _inCarousel; }
    void toggleCarousel();

private:
    AppRegistry();
    ~AppRegistry();

    void renderCarousel(RenderEngine& gfx);

    std::vector<AppBase*> _apps;
    size_t _activeIndex = 0;
    size_t _carouselIndex = 0;
    bool _inCarousel = false;
    SemaphoreHandle_t _registryMutex = nullptr;
};

// Automatic App Registration Macro
#define REGISTER_APP(AppClass) \
    namespace { \
        struct AppClass##Registrar { \
            AppClass##Registrar() { \
                AppRegistry::getInstance().registerApp(new AppClass()); \
            } \
        }; \
        static AppClass##Registrar _registrar_##AppClass; \
    }
