#pragma once
#include <Arduino.h>
#include <SD.h>
#include "src/display/render_engine.h"

// ==============================================================================
// ORBIT v2 — SD Card & Bitmap Asset Loader with SPI Arbitration
// ==============================================================================

class BitmapLoader {
public:
    static BitmapLoader& getInstance() {
        static BitmapLoader instance;
        return instance;
    }

    bool initSdCard();
    bool isSdMounted() const { return _sdMounted; }

    // Draws a 16-bit RGB565 or 24-bit BMP image directly to the TFT screen
    bool drawBmpFile(const char* filepath, int16_t x, int16_t y);

    // Lists files in an SD directory
    std::vector<String> listFiles(const char* dirPath);

    // Returns free & total SD space in MB
    void getStorageStats(uint32_t& totalMb, uint32_t& usedMb);

private:
    BitmapLoader() = default;
    ~BitmapLoader() = default;

    bool _sdMounted = false;
};
