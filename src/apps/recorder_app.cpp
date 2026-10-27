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
    gfx.drawHeader("VOICE RECORDER", 4);

    bool recording = MicSensor::getInstance().isRecording();

    // Central Card (starts at y=40, height 56 -> ends at y=96)
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 26, 112, 56, COLOR_SLATE_MID, recording ? COLOR_NEON_RED : COLOR_SLATE_BORDER);

    if (recording) {
        uint32_t durMs = MicSensor::getInstance().getRecordingDurationMs();
        uint32_t sec = durMs / 1000;

        gfx.fillCircle(20, STATUS_BAR_HEIGHT + 37, 4, COLOR_NEON_RED);
        gfx.drawText(30, STATUS_BAR_HEIGHT + 34, "RECORDING", COLOR_NEON_RED, 1);

        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu.%lu", sec / 60, sec % 60, (durMs % 1000) / 100);
        gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 49, timeBuf, COLOR_TEXT_LIGHT, 2);

        // Live mic level indicator inside card
        TelemetryData telem = SharedData::getInstance().getData();
        uint8_t lvl = constrain((uint8_t)map((int)telem.micDb, 35, 90, 0, 100), 0, 100);
        gfx.drawProgressBar(16, STATUS_BAR_HEIGHT + 71, 96, 4, lvl, COLOR_NEON_MINT);
    } else {
        gfx.drawIcon(20, STATUS_BAR_HEIGHT + 33, 5, COLOR_NEON_CYAN, COLOR_SLATE_MID);
        gfx.drawText(42, STATUS_BAR_HEIGHT + 37, "READY", COLOR_TEXT_LIGHT, 1);

        char countBuf[24];
        snprintf(countBuf, sizeof(countBuf), "%lu Memos Saved", (unsigned long)_memoCount);
        gfx.drawText(16, STATUS_BAR_HEIGHT + 61, countBuf, COLOR_TEXT_MUTED, 1);
    }

    // Storage card (starts at y=96, height 26 -> ends at y=122)
    uint32_t totalMb = 0, usedMb = 0;
    BitmapLoader::getInstance().getStorageStats(totalMb, usedMb);
    char sdBuf[32];
    if (totalMb > 0) {
        uint32_t freeMb = (totalMb >= usedMb) ? (totalMb - usedMb) : 0;
        uint8_t freePct = (uint8_t)(((uint64_t)freeMb * 100) / totalMb);
        snprintf(sdBuf, sizeof(sdBuf), "SD: %luMB (%d%% Free)", (unsigned long)freeMb, (int)freePct);
    } else {
        snprintf(sdBuf, sizeof(sdBuf), "SD: Active (SPI)");
    }
    gfx.drawCard(8, STATUS_BAR_HEIGHT + 82, 112, 26, COLOR_SLATE_MID, COLOR_SLATE_BORDER);
    gfx.drawTextCentered(64, STATUS_BAR_HEIGHT + 91, sdBuf, COLOR_TEXT_MUTED, 1);

    // Controls hint
    const char* hint = recording ? "ENTER: Stop Record" : "ENTER: Start Record";
    gfx.drawTextCentered(64, SCREEN_HEIGHT - 10, hint, recording ? COLOR_NEON_RED : COLOR_TEXT_MUTED, 1);
}

bool RecorderApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK && (evt.button == BTN_ID_ENTER || evt.button == BTN_ID_SELECT)) {
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
