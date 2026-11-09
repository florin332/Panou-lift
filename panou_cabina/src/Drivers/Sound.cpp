// Drivers/Sound.cpp
// Native RP2040 I2S playback for the Marble Pico + MAX98357A.

#include "Sound.h"
#include "Alerts.h"
#include "Settings.h"

#include <I2S.h>
#include <algorithm>
#include <atomic>
#include <cstdint>

namespace
{
    constexpr int I2S_BCLK = 8;
    constexpr int I2S_DATA = 10;
    constexpr int AUDIO_SAMPLE_RATE = 16000;
    constexpr size_t DMA_BUFFER_COUNT = 8;
    constexpr size_t DMA_BUFFER_WORDS = 256;
    constexpr size_t DMA_QUEUE_BYTES = DMA_BUFFER_COUNT * DMA_BUFFER_WORDS * sizeof(uint32_t);
    constexpr size_t SD_STREAM_RING_SAMPLES = 8192;
    constexpr uint32_t SD_STREAM_PREBUFFER_SAMPLES = 4096;

    struct WavClip {
        const uint8_t *samples = nullptr;
        uint32_t sampleCount = 0;
        bool valid = false;
    };

    I2S i2s(OUTPUT, I2S_BCLK, I2S_DATA);
    int16_t sdStreamRing[SD_STREAM_RING_SAMPLES] = {};
    std::atomic<uint32_t> sdStreamReadIndex{0};
    std::atomic<uint32_t> sdStreamWriteIndex{0};
    std::atomic<bool> sdStreamActive{false};
    std::atomic<bool> sdStreamFinished{false};
    WavClip alarmClip;
    WavClip overloadClip;
    WavClip stoppedClip;
    uint32_t audioFrames[DMA_BUFFER_WORDS] = {};
    const WavClip *activeClip = nullptr;
    uint32_t playbackOffset = 0;
    size_t outputBufferOffset = 0;
    float currentGain = 1.0f;
    bool initialized = false;
    bool playbackActive = false;
    bool sdStreamPlaying = false;

    uint16_t readLe16(const uint8_t *data) {
        return static_cast<uint16_t>(data[0]) |
               static_cast<uint16_t>(data[1] << 8);
    }

    uint32_t readLe32(const uint8_t *data) {
        return static_cast<uint32_t>(data[0]) |
               (static_cast<uint32_t>(data[1]) << 8) |
               (static_cast<uint32_t>(data[2]) << 16) |
               (static_cast<uint32_t>(data[3]) << 24);
    }

    bool chunkIdIs(const uint8_t *data, const char *id) {
        return data[0] == static_cast<uint8_t>(id[0]) &&
               data[1] == static_cast<uint8_t>(id[1]) &&
               data[2] == static_cast<uint8_t>(id[2]) &&
               data[3] == static_cast<uint8_t>(id[3]);
    }

    bool parseWav(const uint8_t *data, uint32_t length, WavClip &clip) {
        clip = {};
        if (data == nullptr || length < 12 || !chunkIdIs(data, "RIFF") ||
            !chunkIdIs(data + 8, "WAVE")) {
            return false;
        }

        const uint32_t riffSize = readLe32(data + 4);
        if (riffSize < 4 || riffSize > length - 8) return false;
        const uint32_t riffEnd = riffSize + 8;
        uint32_t offset = 12;
        bool validFormat = false;
        const uint8_t *sampleData = nullptr;
        uint32_t sampleBytes = 0;

        while (offset <= riffEnd && riffEnd - offset >= 8) {
            const uint8_t *chunk = data + offset;
            const uint32_t chunkSize = readLe32(chunk + 4);
            const uint32_t chunkDataOffset = offset + 8;
            if (chunkSize > riffEnd - chunkDataOffset) return false;

            if (chunkIdIs(chunk, "fmt ")) {
                if (chunkSize < 16) return false;
                const uint8_t *format = data + chunkDataOffset;
                validFormat = readLe16(format) == 1 &&
                              readLe16(format + 2) == 1 &&
                              readLe32(format + 4) == AUDIO_SAMPLE_RATE &&
                              readLe32(format + 8) == AUDIO_SAMPLE_RATE * 2 &&
                              readLe16(format + 12) == 2 &&
                              readLe16(format + 14) == 16;
            } else if (chunkIdIs(chunk, "data")) {
                sampleData = data + chunkDataOffset;
                sampleBytes = chunkSize;
            }

            const uint32_t paddedChunkSize = chunkSize + (chunkSize & 1U);
            if (paddedChunkSize > riffEnd - chunkDataOffset) break;
            offset = chunkDataOffset + paddedChunkSize;
        }

        if (!validFormat || sampleData == nullptr || sampleBytes < 2 || (sampleBytes & 1U)) {
            return false;
        }

        clip.samples = sampleData;
        clip.sampleCount = sampleBytes / 2;
        clip.valid = true;
        return true;
    }

    int16_t applyGain(int16_t sample) {
        int32_t scaled = static_cast<int32_t>(static_cast<float>(sample) * currentGain);
        if (scaled > INT16_MAX) scaled = INT16_MAX;
        if (scaled < INT16_MIN) scaled = INT16_MIN;
        return static_cast<int16_t>(scaled);
    }

    void fillDmaQueue() {
        if (!initialized) return;

        if (sdStreamActive.load(std::memory_order_acquire) && !sdStreamPlaying) {
            playbackActive = false;
            activeClip = nullptr;

            const uint32_t writeIndex = sdStreamWriteIndex.load(std::memory_order_acquire);
            const uint32_t readIndex = sdStreamReadIndex.load(std::memory_order_relaxed);
            const uint32_t bufferedSamples = writeIndex - readIndex;
            if (bufferedSamples < SD_STREAM_PREBUFFER_SAMPLES &&
                !sdStreamFinished.load(std::memory_order_acquire)) {
                return;
            }
            if (i2s.availableForWrite() < static_cast<int>(DMA_QUEUE_BYTES)) return;

            outputBufferOffset = 0;
            sdStreamPlaying = true;
        }

        while (true) {
            const int availableBytes = i2s.availableForWrite();
            if (availableBytes < static_cast<int>(sizeof(uint32_t))) return;

            const size_t availableFrames = static_cast<size_t>(availableBytes) / sizeof(uint32_t);
            const size_t bufferFramesRemaining = DMA_BUFFER_WORDS - outputBufferOffset;
            const size_t framesToWrite = std::min(availableFrames, bufferFramesRemaining);
            if (framesToWrite == 0) return;

            if (sdStreamPlaying) {
                const uint32_t readIndex = sdStreamReadIndex.load(std::memory_order_relaxed);
                const uint32_t writeIndex = sdStreamWriteIndex.load(std::memory_order_acquire);
                const uint32_t bufferedSamples = writeIndex - readIndex;
                if (bufferedSamples == 0) {
                    if (sdStreamFinished.load(std::memory_order_acquire)) {
                        sdStreamPlaying = false;
                        sdStreamActive.store(false, std::memory_order_release);
                    }
                    return;
                }

                const size_t streamFrames = std::min(framesToWrite, static_cast<size_t>(bufferedSamples));
                for (size_t i = 0; i < streamFrames; i++) {
                    const int16_t sample = applyGain(sdStreamRing[(readIndex + i) % SD_STREAM_RING_SAMPLES]);
                    const uint16_t packedSample = static_cast<uint16_t>(sample);
                    audioFrames[i] = (static_cast<uint32_t>(packedSample) << 16) | packedSample;
                }

                const size_t writtenBytes = i2s.write(
                    reinterpret_cast<const uint8_t *>(audioFrames),
                    streamFrames * sizeof(uint32_t));
                const size_t writtenFrames = writtenBytes / sizeof(uint32_t);
                sdStreamReadIndex.store(readIndex + static_cast<uint32_t>(writtenFrames),
                                        std::memory_order_release);
                outputBufferOffset = (outputBufferOffset + writtenFrames) % DMA_BUFFER_WORDS;
                if (writtenFrames < streamFrames) return;
                continue;
            }

            size_t audioFramesToWrite = 0;
            if (playbackActive && activeClip != nullptr) {
                const uint32_t samplesRemaining = activeClip->sampleCount - playbackOffset;
                audioFramesToWrite = std::min(framesToWrite, static_cast<size_t>(samplesRemaining));
            } else if (outputBufferOffset == 0) {
                return;
            }

            for (size_t i = 0; i < audioFramesToWrite; i++) {
                const uint8_t *sampleBytes = activeClip->samples + playbackOffset * 2;
                const int16_t sample = applyGain(static_cast<int16_t>(readLe16(sampleBytes)));
                const uint16_t packedSample = static_cast<uint16_t>(sample);
                audioFrames[i] = (static_cast<uint32_t>(packedSample) << 16) | packedSample;
                playbackOffset++;
            }
            for (size_t i = audioFramesToWrite; i < framesToWrite; i++) {
                audioFrames[i] = 0;
            }

            const size_t writtenBytes = i2s.write(
                reinterpret_cast<const uint8_t *>(audioFrames),
                framesToWrite * sizeof(uint32_t));
            const size_t writtenFrames = writtenBytes / sizeof(uint32_t);
            outputBufferOffset = (outputBufferOffset + writtenFrames) % DMA_BUFFER_WORDS;

            if (writtenFrames < audioFramesToWrite) {
                playbackOffset -= static_cast<uint32_t>(audioFramesToWrite - writtenFrames);
                return;
            }

            if (playbackActive && activeClip != nullptr && playbackOffset == activeClip->sampleCount) {
                playbackActive = false;
                activeClip = nullptr;
            }

            if (writtenFrames < framesToWrite) return;
        }
    }

    void playClip(const WavClip &clip) {
        if (!initialized || !clip.valid) return;
        sdStreamActive.store(false, std::memory_order_release);
        sdStreamPlaying = false;
        activeClip = &clip;
        playbackOffset = 0;
        playbackActive = true;
        fillDmaQueue();
    }
}

namespace Sound
{
    void init() {
        if (initialized) return;

        parseWav(alarm_wav, alarm_wav_len, alarmClip);
        parseWav(overload_wav, overload_wav_len, overloadClip);
        parseWav(stop_wav, stop_wav_len, stoppedClip);

        currentGain = Settings::audioGain();
        i2s.setSysClk(AUDIO_SAMPLE_RATE);
        i2s.setBCLK(I2S_BCLK);
        i2s.setDATA(I2S_DATA);
        i2s.setBitsPerSample(16);
        i2s.setBuffers(DMA_BUFFER_COUNT, DMA_BUFFER_WORDS);
        initialized = i2s.begin(AUDIO_SAMPLE_RATE);
    }

    void update() {
        fillDmaQueue();
    }

    bool beginSdStream() {
        if (!initialized) return false;
        sdStreamReadIndex.store(0, std::memory_order_relaxed);
        sdStreamWriteIndex.store(0, std::memory_order_relaxed);
        sdStreamFinished.store(false, std::memory_order_relaxed);
        sdStreamActive.store(true, std::memory_order_release);
        return true;
    }

    size_t sdStreamFreeSamples() {
        if (!sdStreamActive.load(std::memory_order_acquire)) return 0;
        const uint32_t writeIndex = sdStreamWriteIndex.load(std::memory_order_relaxed);
        const uint32_t readIndex = sdStreamReadIndex.load(std::memory_order_acquire);
        const uint32_t usedSamples = writeIndex - readIndex;
        return usedSamples >= SD_STREAM_RING_SAMPLES ? 0 : SD_STREAM_RING_SAMPLES - usedSamples;
    }

    size_t pushSdStreamSamples(const int16_t *samples, size_t count) {
        if (samples == nullptr || !sdStreamActive.load(std::memory_order_acquire)) return 0;

        const uint32_t writeIndex = sdStreamWriteIndex.load(std::memory_order_relaxed);
        const uint32_t readIndex = sdStreamReadIndex.load(std::memory_order_acquire);
        const uint32_t usedSamples = writeIndex - readIndex;
        const size_t freeSamples = usedSamples >= SD_STREAM_RING_SAMPLES
            ? 0
            : SD_STREAM_RING_SAMPLES - usedSamples;
        const size_t samplesToWrite = std::min(count, freeSamples);

        for (size_t i = 0; i < samplesToWrite; i++) {
            sdStreamRing[(writeIndex + i) % SD_STREAM_RING_SAMPLES] = samples[i];
        }
        sdStreamWriteIndex.store(writeIndex + static_cast<uint32_t>(samplesToWrite),
                                 std::memory_order_release);
        return samplesToWrite;
    }

    void finishSdStream() {
        sdStreamFinished.store(true, std::memory_order_release);
    }

    bool isSdStreamActive() {
        return sdStreamActive.load(std::memory_order_acquire);
    }

    void alarm() {
        playClip(alarmClip);
    }

    void overload() {
        playClip(overloadClip);
    }

    void liftStopped() {
        playClip(stoppedClip);
    }

    void stop() {
        playbackActive = false;
        activeClip = nullptr;
        sdStreamActive.store(false, std::memory_order_release);
        sdStreamPlaying = false;
        fillDmaQueue();
    }

    bool isPlaying() {
        return initialized &&
               (playbackActive || sdStreamActive.load(std::memory_order_acquire) ||
            sdStreamPlaying || i2s.availableForWrite() < static_cast<int>(DMA_QUEUE_BYTES));
    }

    bool setGain(float gain) {
        if (!Settings::setAudioGain(gain)) return false;
        currentGain = Settings::audioGain();
        return true;
    }

    float gain() {
        return Settings::audioGain();
    }
}