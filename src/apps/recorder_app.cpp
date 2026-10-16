#include "src/apps/recorder_app.h"
#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/sensors/mic_sensor.h"
#include "src/display/bitmap_loader.h"
#include "src/display/theme.h"

REGISTER_APP(RecorderApp);

void RecorderApp::onInit() {}

void RecorderApp::onEnter() {
    std::vector<String> files = BitmapLoader::getInstance().listFiles(SD_PATH_MEMOS);
    _memoCount = files.size();
}

void RecorderApp::onExit() {
    if (MicSensor::getInstance().isRecording()) {
        MicSensor::getInstance().stopRecording();
    }
}

void RecorderApp::onUpdate(uint32_t deltaMs) {}

void RecorderApp::onRender(RenderEngine& gfx) {
    gfx.fillRect(0, STATUS_BAR_HEIGHT, SCREEN_WIDTH, CONTENT_HEIGHT, COLOR_VOID_DARK);
    gfx.drawHeader("VOICE RECORDER", 5);

    bool recording = MicSensor::getInstance().isRecording();

    // Central Card
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 30, 112, 58, COLOR_SLATE_MID, recording ? COLOR_NEON_RED : COLOR_SLATE_BORDER);

    if (recording) {
        uint32_t durMs = MicSensor::getInstance().getRecordingDurationMs();
        uint32_t sec = durMs / 1000;

        gfx.fillCircle(20, STATUS_BAR_HEIGHT + 44, 4, COLOR_NEON_RED);
        gfx.drawText(30, STATUS_BAR_HEIGHT + 41, "RECORDING", COLOR_NEON_RED, 1);

        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu.%lu", sec / 60, sec % 60, (durMs % 1000) / 100);
        gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 58, timeBuf, COLOR_TEXT_LIGHT, 2);

        // Live mic level indicator inside card
        TelemetryData telem = SharedData::getInstance().getData();
        uint8_t lvl = constrain((uint8_t)map((int)telem.micDb, 35, 90, 0, 100), 0, 100);
        gfx.drawProgressBar(16, STATUS_BAR_HEIGHT + 78, 96, 4, lvl, COLOR_NEON_MINT);
    } else {
        gfx.drawIcon(20, STATUS_BAR_HEIGHT + 36, 5, COLOR_NEON_CYAN, COLOR_SLATE_MID);
        gfx.drawText(42, STATUS_BAR_HEIGHT + 40, "READY", COLOR_TEXT_LIGHT, 1);

        char countBuf[24];
        snprintf(countBuf, sizeof(countBuf), "%lu Memos Saved", (unsigned long)_memoCount);
        gfx.drawText(16, STATUS_BAR_HEIGHT + 64, countBuf, COLOR_TEXT_MUTED, 1);
    }

    // Storage card
    uint32_t totalMb = 0, usedMb = 0;
    BitmapLoader::getInstance().getStorageStats(totalMb, usedMb);
    char sdBuf[32];
    if (totalMb > 0) {
        snprintf(sdBuf, sizeof(sdBuf), "SD: %lu MB / %lu MB", (unsigned long)usedMb, (unsigned long)totalMb);
    } else {
        snprintf(sdBuf, sizeof(sdBuf), "SD: OK (Shared SPI)");
    }
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 94, 112, 24, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 101, sdBuf, COLOR_TEXT_MUTED, 1);

    // Controls hint
    const char* hint = recording ? "SELECT: Stop Record" : "SELECT: Start Record";
    gfx.drawTextCentered(64, SCREEN_HEIGHT - 12, hint, recording ? COLOR_NEON_RED : COLOR_TEXT_MUTED, 1);
}

bool RecorderApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK && evt.button == BTN_ID_SELECT) {
        if (MicSensor::getInstance().isRecording()) {
            MicSensor::getInstance().stopRecording();
            onEnter(); // Refresh count
        } else {
            MicSensor::getInstance().startRecording();
        }
        return true;
    }
    return false;
}
