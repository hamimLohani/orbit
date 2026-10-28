#pragma once
#include <Arduino.h>
#include <ESP_I2S.h>
#include <FS.h>
#include <SD.h>
#include "include/config.h"

// ==============================================================================
// ORBIT v2 — INMP441 I2S Microphone, 16-Band FFT & Voice Memo Recorder
// ==============================================================================

enum MemoMode {
    MEMO_MODE_VOICE_TO_TEXT = 0, // Transcribe & Web Copy
    MEMO_MODE_ASK_AI,            // Voice Question -> AI Answer on Screen
    MEMO_MODE_AUDIO_WAV,         // Standard 16kHz PCM WAV Audio
    MEMO_MODE_AI_TASKS,          // AI Action Item & Task Extractor
    MEMO_MODE_COUNT
};

class MicSensor {
public:
    static MicSensor& getInstance() {
        static MicSensor instance;
        return instance;
    }

    void init();
    void update(); // Called frequently from Core 0 audio task

    // Audio Telemetry
    float getRms() const { return _rms; }
    float getPeak() const { return _peak; }
    float getDb() const { return _db; }
    const uint8_t* getFftBands() const { return _fftBands; }

    // Voice Memo Recording
    bool startRecording(const char* filename = nullptr);
    void stopRecording();
    bool isRecording() const { return _isRecording; }
    uint32_t getRecordingDurationMs() const;
    const char* getCurrentRecordingPath() const { return _currentMemoPath; }

    // Memo Mode (Multiple Options)
    void setMemoMode(MemoMode mode) { _memoMode = mode; }
    MemoMode getMemoMode() const { return _memoMode; }
    const char* getMemoModeName() const;
    const char* getMemoModeTag() const;

private:
    MicSensor();
    ~MicSensor() = default;

    void processFft(const int16_t* samples, size_t count);
    void writeWavHeader(File& file, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t channels);
    void updateWavHeader(File& file, uint32_t totalAudioBytes);

    I2SClass _i2s;
    float _rms = 0.0f;
    float _peak = 0.0f;
    float _db = 35.0f;
    uint8_t _fftBands[FFT_NUM_BINS] = {0};

    // Voice memo recording state
    bool _isRecording = false;
    MemoMode _memoMode = MEMO_MODE_VOICE_TO_TEXT;
    File _wavFile;
    char _currentMemoPath[64];
    uint32_t _recordStartTime = 0;
    uint32_t _recordedBytes = 0;
};
