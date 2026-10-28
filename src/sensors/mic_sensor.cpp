#include "src/sensors/mic_sensor.h"
#include "src/core/event_bus.h"
#include "src/core/settings_manager.h"
#include "src/display/render_engine.h"
#include <cmath>

MicSensor::MicSensor() {
    memset(_fftBands, 0, sizeof(_fftBands));
    _currentMemoPath[0] = '\0';
}

void MicSensor::init() {
    _i2s.setPins(PIN_I2S_SCK, PIN_I2S_WS, -1, PIN_I2S_SD);
    // Standard 16kHz 16-bit mono
    _i2s.begin(I2S_MODE_STD, I2S_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
    _i2s.setTimeout(5); // 5ms maximum timeout to prevent blocking
}

void MicSensor::update() {
    if (_i2s.available() < (int)(sizeof(int16_t) * 32)) {
        return; // Non-blocking: skip if buffer doesn't have enough samples yet
    }

    int16_t samples[I2S_BUFFER_SIZE];
    size_t bytesRead = _i2s.readBytes((char*)samples, sizeof(samples));
    size_t sampleCount = bytesRead / sizeof(int16_t);

    if (sampleCount == 0) return;

    // Stream to WAV file if recording
    if (_isRecording && _wavFile) {
        SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            digitalWrite(PIN_TFT_CS, HIGH); // Ensure TFT deselected
            _wavFile.write((const uint8_t*)samples, bytesRead);
            _recordedBytes += bytesRead;
            xSemaphoreGive(mutex);
        }
    }

    // Compute Peak and RMS
    int64_t sumSquare = 0;
    int16_t peakVal = 0;

    for (size_t i = 0; i < sampleCount; ++i) {
        int16_t val = abs(samples[i]);
        if (val > peakVal) peakVal = val;
        sumSquare += (int64_t)val * val;
    }

    float rms = sqrt((float)sumSquare / sampleCount);
    _rms = (_rms * 0.7f) + (rms * 0.3f);
    _peak = (_peak * 0.6f) + (peakVal * 0.4f);

    // Approximate SPL dB (ambient baseline 35dB, max ~95dB)
    float rawDb = 20.0f * log10f((_rms > 1.0f ? _rms : 1.0f) / 32767.0f) + 95.0f;
    if (rawDb < 30.0f) rawDb = 30.0f;
    if (rawDb > 105.0f) rawDb = 105.0f;
    _db = (_db * 0.65f) + (rawDb * 0.35f);

    // Check noise threshold from settings
    float noiseTh = SettingsManager::getInstance().getNoiseThreshold();
    if (_db > noiseTh) {
        EventBus::getInstance().publish(EVENT_NOISE_ALERT, &_db);
    }

    // Process 16 Equalizer FFT Bands
    processFft(samples, sampleCount);
}

void MicSensor::processFft(const int16_t* samples, size_t count) {
    // 16 Frequency Equalizer Bins
    int samplesPerBin = count / FFT_NUM_BINS;
    if (samplesPerBin < 1) return;

    for (int bin = 0; bin < FFT_NUM_BINS; ++bin) {
        int64_t binEnergy = 0;
        int start = bin * samplesPerBin;
        for (int i = 0; i < samplesPerBin; ++i) {
            binEnergy += abs(samples[start + i]);
        }
        int avg = binEnergy / samplesPerBin;

        // Exponential weighting for higher frequencies (compensate natural pink noise drop)
        float weight = 1.0f + (bin * 0.25f);
        int scaled = (int)((avg * weight) / 80);
        if (scaled > 255) scaled = 255;

        // Peak decay smoothing
        if (scaled > _fftBands[bin]) {
            _fftBands[bin] = scaled;
        } else {
            _fftBands[bin] = (_fftBands[bin] * 4) / 5; // Fast decay
        }
    }

    // Detect strong beat / sound spike on sub-bass / bass bins
    if (_fftBands[0] > 180 || _fftBands[1] > 190) {
        EventBus::getInstance().publish(EVENT_SOUND_SPIKE, nullptr);
    }
}

bool MicSensor::startRecording(const char* filename) {
    if (_isRecording) return true;

    SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }

    digitalWrite(PIN_TFT_CS, HIGH);

    if (filename) {
        snprintf(_currentMemoPath, sizeof(_currentMemoPath), "%s/%s", SD_PATH_MEMOS, filename);
    } else {
        snprintf(_currentMemoPath, sizeof(_currentMemoPath), "%s/memo_%lu.wav", SD_PATH_MEMOS, millis());
    }

    _wavFile = SD.open(_currentMemoPath, FILE_WRITE);
    if (!_wavFile) {
        xSemaphoreGive(mutex);
        return false;
    }

    // Write empty WAV header placeholder
    writeWavHeader(_wavFile, I2S_SAMPLE_RATE, 16, 1);
    xSemaphoreGive(mutex);

    _isRecording = true;
    _recordedBytes = 0;
    _recordStartTime = millis();

    EventBus::getInstance().publish(EVENT_RECORDING_START, _currentMemoPath);
    return true;
}

void MicSensor::stopRecording() {
    if (!_isRecording) return;
    _isRecording = false;

    SemaphoreHandle_t mutex = RenderEngine::getSpiMutex();
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        digitalWrite(PIN_TFT_CS, HIGH);
        if (_wavFile) {
            updateWavHeader(_wavFile, _recordedBytes);
            _wavFile.close();
        }
        xSemaphoreGive(mutex);
    }

    EventBus::getInstance().publish(EVENT_RECORDING_STOP, _currentMemoPath);
}

uint32_t MicSensor::getRecordingDurationMs() const {
    if (!_isRecording) return 0;
    return millis() - _recordStartTime;
}

void MicSensor::writeWavHeader(File& file, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t channels) {
    uint8_t header[44] = {
        'R', 'I', 'F', 'F',
        0, 0, 0, 0, // Placeholder for chunkSize
        'W', 'A', 'V', 'E',
        'f', 'm', 't', ' ',
        16, 0, 0, 0, // Subchunk1Size (16 for PCM)
        1, 0,        // AudioFormat (1 for PCM)
        (uint8_t)channels, 0,
        (uint8_t)(sampleRate & 0xFF), (uint8_t)((sampleRate >> 8) & 0xFF), (uint8_t)((sampleRate >> 16) & 0xFF), (uint8_t)((sampleRate >> 24) & 0xFF),
        0, 0, 0, 0, // Placeholder byteRate
        (uint8_t)(channels * bitsPerSample / 8), 0, // blockAlign
        (uint8_t)bitsPerSample, 0,
        'd', 'a', 't', 'a',
        0, 0, 0, 0  // Placeholder dataSize
    };

    uint32_t byteRate = sampleRate * channels * (bitsPerSample / 8);
    header[28] = byteRate & 0xFF;
    header[29] = (byteRate >> 8) & 0xFF;
    header[30] = (byteRate >> 16) & 0xFF;
    header[31] = (byteRate >> 24) & 0xFF;

    file.write(header, 44);
}

void MicSensor::updateWavHeader(File& file, uint32_t totalAudioBytes) {
    uint32_t chunkSize = totalAudioBytes + 36;
    file.seek(4);
    file.write((uint8_t*)&chunkSize, 4);

    file.seek(40);
    file.write((uint8_t*)&totalAudioBytes, 4);
}

const char* MicSensor::getMemoModeName() const {
    switch (_memoMode) {
        case MEMO_MODE_VOICE_TO_TEXT: return "VOICE TO TEXT";
        case MEMO_MODE_ASK_AI:        return "ASK AI (WIFI)";
        case MEMO_MODE_AUDIO_WAV:     return "AUDIO WAV";
        case MEMO_MODE_AI_TASKS:      return "AI TASKS";
        default:                      return "VOICE TO TEXT";
    }
}

const char* MicSensor::getMemoModeTag() const {
    switch (_memoMode) {
        case MEMO_MODE_VOICE_TO_TEXT: return "Transcribe & Web Copy";
        case MEMO_MODE_ASK_AI:        return "Voice Query -> AI Screen";
        case MEMO_MODE_AUDIO_WAV:     return "16kHz PCM to SD";
        case MEMO_MODE_AI_TASKS:      return "Notes & Action Items";
        default:                      return "";
    }
}
