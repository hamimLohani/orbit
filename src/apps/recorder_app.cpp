#include "src/apps/recorder_app.h"
#include "src/core/app_registry.h"
#include "src/core/shared_data.h"
#include "src/sensors/mic_sensor.h"
#include "src/display/bitmap_loader.h"
#include "src/display/theme.h"
#include "src/display/render_engine.h"

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
    gfx.drawHeader("VOICE MEMO", 4);

    bool recording = MicSensor::getInstance().isRecording();
    MemoMode mode = MicSensor::getInstance().getMemoMode();

    // --------------------------------------------------------------------------
    // Card 0: Mode / Option Selector (y=39, h=30, w=116, x=6)
    // --------------------------------------------------------------------------
    uint16_t optColor = (mode == MEMO_MODE_VOICE_TO_TEXT) ? COLOR_NEON_CYAN :
                        (mode == MEMO_MODE_ASK_AI)        ? COLOR_NEON_PURPLE :
                        (mode == MEMO_MODE_AUDIO_WAV)     ? COLOR_NEON_MINT : COLOR_NEON_AMBER;

    gfx.drawCard(6, 39, 116, 30, COLOR_SLATE_MID, recording ? COLOR_SLATE_BORDER : optColor);

    gfx.drawText(10, 44, "MODE:", COLOR_TEXT_MUTED, 1);

    const char* modeLabel = (mode == MEMO_MODE_VOICE_TO_TEXT) ? "VOICE->TXT" :
                            (mode == MEMO_MODE_ASK_AI)        ? "ASK AI" :
                            (mode == MEMO_MODE_AUDIO_WAV)     ? "AUDIO WAV" : "AI TASKS";
    gfx.drawTextRight(118, 44, modeLabel, optColor, 1);

    const char* modeTag = (mode == MEMO_MODE_VOICE_TO_TEXT) ? "< Live Dictate >" :
                          (mode == MEMO_MODE_ASK_AI)        ? "< Ask Gemini >" :
                          (mode == MEMO_MODE_AUDIO_WAV)     ? "< 16k WAV to SD >" :
                                                              "< Action Items >";
    gfx.drawTextCentered(64, 56, modeTag, recording ? COLOR_TEXT_MUTED : COLOR_TEXT_LIGHT, 1);

    // --------------------------------------------------------------------------
    // Card 1: Recording & Live Level HUD (y=72, h=48, w=116, x=6)
    // --------------------------------------------------------------------------
    uint16_t c1Border = recording ? (((millis() / 250) % 2 == 0) ? COLOR_NEON_RED : COLOR_WHITE) : COLOR_SLATE_BORDER;
    gfx.drawCard(6, 72, 116, 48, COLOR_SLATE_MID, c1Border);

    if (recording) {
        uint32_t durMs = MicSensor::getInstance().getRecordingDurationMs();
        uint32_t sec = durMs / 1000;

        gfx.fillCircle(14, 79, 3, COLOR_NEON_RED);
        gfx.drawText(22, 76, "RECORDING...", COLOR_NEON_RED, 1);

        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02lu:%02lu", (unsigned long)(sec / 60), (unsigned long)(sec % 60));
        gfx.drawTextCentered(64, 87, timeBuf, COLOR_TEXT_LIGHT, 2);

        // Live mic level indicator inside card
        TelemetryData telem = SharedData::getInstance().getData();
        uint8_t lvl = constrain((uint8_t)map((int)telem.micDb, 35, 90, 0, 100), 0, 100);
        gfx.drawProgressBar(10, 108, 108, 4, lvl, COLOR_NEON_MINT);
    } else {
        gfx.drawText(10, 77, "STATUS", COLOR_TEXT_MUTED, 1);

        char countBuf[20];
        snprintf(countBuf, sizeof(countBuf), "%lu Memos", (unsigned long)_memoCount);
        gfx.drawTextRight(118, 77, countBuf, COLOR_NEON_CYAN, 1);

        gfx.drawTextCentered(64, 89, "READY TO RECORD", COLOR_TEXT_LIGHT, 1);

        const char* targetMsg = (mode == MEMO_MODE_VOICE_TO_TEXT) ? "Transcribe on Stop" :
                                (mode == MEMO_MODE_ASK_AI)        ? "Streams AI Answer" :
                                (mode == MEMO_MODE_AUDIO_WAV)     ? "Direct SD Storage" :
                                                                    "Auto-Extract Tasks";
        gfx.drawTextCentered(64, 103, targetMsg, optColor, 1);
    }

    // --------------------------------------------------------------------------
    // Card 2: Storage & Web Status (y=123, h=18, w=116, x=6)
    // --------------------------------------------------------------------------
    gfx.drawCard(6, 123, 116, 18, COLOR_SLATE_MID, COLOR_SLATE_BORDER);

    uint32_t totalMb = 0, usedMb = 0;
    BitmapLoader::getInstance().getStorageStats(totalMb, usedMb);
    char sdBuf[20];
    if (totalMb > 0) {
        uint32_t freeMb = (totalMb >= usedMb) ? (totalMb - usedMb) : 0;
        snprintf(sdBuf, sizeof(sdBuf), "SD:%luMB", (unsigned long)freeMb);
    } else {
        snprintf(sdBuf, sizeof(sdBuf), "SD:Ready");
    }
    gfx.drawText(10, 128, sdBuf, COLOR_TEXT_MUTED, 1);

    TelemetryData telem = SharedData::getInstance().getData();
    if (telem.wifiConnected) {
        gfx.drawTextRight(118, 128, "WIFI:ON", COLOR_NEON_MINT, 1);
    } else {
        gfx.drawTextRight(118, 128, "WIFI:OFF", COLOR_TEXT_MUTED, 1);
    }

    // --------------------------------------------------------------------------
    // Bottom Hint (y=146)
    // --------------------------------------------------------------------------
    if (recording) {
        gfx.drawTextCentered(64, 146, "OK: Stop & Save", COLOR_NEON_RED, 1);
    } else {
        gfx.drawTextCentered(64, 146, "L/R:Mode  OK:Rec", COLOR_TEXT_MUTED, 1);
    }
}

bool RecorderApp::onButtonEvent(const ButtonEvent& evt) {
    if (evt.type == BUTTON_CLICK || evt.type == BUTTON_REPEAT) {
        bool recording = MicSensor::getInstance().isRecording();

        if (evt.button == BTN_ID_LEFT && !recording) {
            MemoMode m = MicSensor::getInstance().getMemoMode();
            m = (m == 0) ? static_cast<MemoMode>(MEMO_MODE_COUNT - 1) : static_cast<MemoMode>(m - 1);
            MicSensor::getInstance().setMemoMode(m);
            RenderEngine::getInstance().showToast("OPTION", MicSensor::getInstance().getMemoModeName(), COLOR_NEON_CYAN, 800);
            return true;
        }

        if (evt.button == BTN_ID_RIGHT && !recording) {
            MemoMode m = MicSensor::getInstance().getMemoMode();
            m = static_cast<MemoMode>((m + 1) % MEMO_MODE_COUNT);
            MicSensor::getInstance().setMemoMode(m);
            RenderEngine::getInstance().showToast("OPTION", MicSensor::getInstance().getMemoModeName(), COLOR_NEON_CYAN, 800);
            return true;
        }

        if (evt.button == BTN_ID_ENTER || evt.button == BTN_ID_SELECT) {
            if (recording) {
                MemoMode m = MicSensor::getInstance().getMemoMode();
                MicSensor::getInstance().stopRecording();
                onEnter(); // Refresh count

                if (m == MEMO_MODE_VOICE_TO_TEXT) {
                    RenderEngine::getInstance().showToast("VOICE TO TEXT", "Sent to Web! Ready to Copy", COLOR_NEON_CYAN, 2500);
                } else if (m == MEMO_MODE_ASK_AI) {
                    if (SharedData::getInstance().getData().wifiConnected) {
                        RenderEngine::getInstance().showToast("ASK AI QUERY", "Transmitting to AI...", COLOR_NEON_PURPLE, 2200);
                    } else {
                        RenderEngine::getInstance().showToast("WI-FI REQUIRED", "Connect Wi-Fi to Ask AI", COLOR_NEON_RED, 2500);
                    }
                } else if (m == MEMO_MODE_AUDIO_WAV) {
                    RenderEngine::getInstance().showToast("AUDIO SAVED", "Saved WAV to SD Card", COLOR_NEON_MINT, 1800);
                } else {
                    RenderEngine::getInstance().showToast("AI NOTES", "Ready on Web Dashboard", COLOR_NEON_AMBER, 2200);
                }
            } else {
                MicSensor::getInstance().startRecording();
                RenderEngine::getInstance().showToast("RECORDING", "Speaking into ORBIT...", COLOR_NEON_RED, 1200);
            }
            return true;
        }
    }
    return false;
}
