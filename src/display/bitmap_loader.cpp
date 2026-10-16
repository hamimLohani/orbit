#include "src/display/bitmap_loader.h"
#include <SPI.h>

bool BitmapLoader::initSdCard() {
    SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Ensure TFT is deselected
        pinMode(PIN_TFT_CS, OUTPUT);
        digitalWrite(PIN_TFT_CS, HIGH);

        // Mount SD
        _sdMounted = SD.begin(PIN_SD_CS, SPI, 20000000); // 20MHz SPI
        if (_sdMounted) {
            // Ensure default folders exist
            if (!SD.exists(SD_PATH_WALLPAPERS)) SD.mkdir(SD_PATH_WALLPAPERS);
            if (!SD.exists(SD_PATH_ICONS)) SD.mkdir(SD_PATH_ICONS);
            if (!SD.exists(SD_PATH_MEMOS)) SD.mkdir(SD_PATH_MEMOS);
            if (!SD.exists(SD_PATH_LOGS)) SD.mkdir(SD_PATH_LOGS);
        }
        xSemaphoreGive(mutex);
    }
    return _sdMounted;
}

std::vector<String> BitmapLoader::listFiles(const char* dirPath) {
    std::vector<String> files;
    if (!_sdMounted) return files;

    SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        digitalWrite(PIN_TFT_CS, HIGH);
        File root = SD.open(dirPath);
        if (root && root.isDirectory()) {
            File entry = root.openNextFile();
            while (entry) {
                if (!entry.isDirectory()) {
                    files.push_back(String(entry.name()));
                }
                entry = root.openNextFile();
            }
        }
        xSemaphoreGive(mutex);
    }
    return files;
}

void BitmapLoader::getStorageStats(uint32_t& totalMb, uint32_t& usedMb) {
    totalMb = 0;
    usedMb = 0;
    if (!_sdMounted) return;

    SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        digitalWrite(PIN_TFT_CS, HIGH);
        uint64_t totalBytes = SD.totalBytes();
        uint64_t usedBytes = SD.usedBytes();
        totalMb = (uint32_t)(totalBytes / (1024 * 1024));
        usedMb = (uint32_t)(usedBytes / (1024 * 1024));
        xSemaphoreGive(mutex);
    }
}

bool BitmapLoader::drawBmpFile(const char* filepath, int16_t x, int16_t y) {
    if (!_sdMounted) return false;

    SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(200)) != pdTRUE) {
        return false;
    }

    // Deselect TFT to read SD file
    digitalWrite(PIN_TFT_CS, HIGH);
    File bmpFile = SD.open(filepath, FILE_READ);
    if (!bmpFile) {
        xSemaphoreGive(mutex);
        return false;
    }

    // Verify BMP Header
    if (bmpFile.read() != 'B' || bmpFile.read() != 'M') {
        bmpFile.close();
        xSemaphoreGive(mutex);
        return false;
    }

    bmpFile.seek(10); // Offset to image data
    uint32_t imageOffset = bmpFile.read() | (bmpFile.read() << 8) | (bmpFile.read() << 16) | (bmpFile.read() << 24);

    bmpFile.seek(18); // Width and Height
    int32_t bmpWidth = bmpFile.read() | (bmpFile.read() << 8) | (bmpFile.read() << 16) | (bmpFile.read() << 24);
    int32_t bmpHeight = bmpFile.read() | (bmpFile.read() << 8) | (bmpFile.read() << 16) | (bmpFile.read() << 24);

    bmpFile.seek(28); // Bit depth
    uint16_t planes = bmpFile.read() | (bmpFile.read() << 8);
    uint16_t depth = bmpFile.read() | (bmpFile.read() << 8);

    if (planes != 1 || (depth != 24 && depth != 16)) {
        bmpFile.close();
        xSemaphoreGive(mutex);
        return false; // Unsupported depth
    }

    bool flip = true;
    if (bmpHeight < 0) {
        bmpHeight = -bmpHeight;
        flip = false;
    }

    // Crop if larger than screen
    int16_t w = min((int16_t)bmpWidth, (int16_t)(SCREEN_WIDTH - x));
    int16_t h = min((int16_t)bmpHeight, (int16_t)(SCREEN_HEIGHT - y));
    if (w <= 0 || h <= 0) {
        bmpFile.close();
        xSemaphoreGive(mutex);
        return false;
    }

    // Row size in bytes padded to 4 bytes
    uint32_t rowSize = (depth == 24) ? ((bmpWidth * 3 + 3) & ~3) : ((bmpWidth * 2 + 3) & ~3);
    uint8_t rowBuffer[rowSize];
    Adafruit_ST7735& tft = RenderEngine::getInstance().getTft();

    for (int16_t row = 0; row < h; row++) {
        uint32_t pos = flip ? (imageOffset + (bmpHeight - 1 - row) * rowSize) : (imageOffset + row * rowSize);
        
        // Read scanline from SD
        digitalWrite(PIN_TFT_CS, HIGH);
        bmpFile.seek(pos);
        bmpFile.read(rowBuffer, rowSize);
        digitalWrite(PIN_SD_CS, HIGH); // Deselect SD

        // Render scanline to TFT
        if (depth == 24) {
            for (int16_t col = 0; col < w; col++) {
                uint8_t b = rowBuffer[col * 3];
                uint8_t g = rowBuffer[col * 3 + 1];
                uint8_t r = rowBuffer[col * 3 + 2];
                uint16_t color = RGB565(r, g, b);
                tft.drawPixel(x + col, y + row, color);
            }
        } else if (depth == 16) {
            for (int16_t col = 0; col < w; col++) {
                uint16_t color = rowBuffer[col * 2] | (rowBuffer[col * 2 + 1] << 8);
                tft.drawPixel(x + col, y + row, color);
            }
        }
    }

    bmpFile.close();
    digitalWrite(PIN_SD_CS, HIGH);
    xSemaphoreGive(mutex);
    return true;
}
