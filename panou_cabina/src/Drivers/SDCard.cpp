#include "SDCard.h"

#include "Pins.h"
#include "Sound.h"

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>

namespace
{
    constexpr uint8_t AUDIO_FILE_CAPACITY = 31;
    constexpr size_t PCM_READ_SAMPLES = 512;
    bool initialized = false;
    std::atomic<bool> sExclusiveRequested{false};
    std::atomic<bool> sExclusiveAck{false};
    SDCard::AudioFileInfo audioFiles[AUDIO_FILE_CAPACITY] = {};
    uint8_t sAudioFileCount = 0;
    File audioTestFile;
    bool audioTestReading = false;
    uint32_t audioDataRemaining = 0;
    uint8_t pcmReadBytes[PCM_READ_SAMPLES * 2] = {};
    int16_t pcmSamples[PCM_READ_SAMPLES] = {};
    size_t pendingSamples = 0;
    size_t pendingSampleOffset = 0;

    uint16_t readLe16(const uint8_t *data)
    {
        return static_cast<uint16_t>(data[0]) |
               static_cast<uint16_t>(data[1] << 8);
    }

    uint32_t readLe32(const uint8_t *data)
    {
        return static_cast<uint32_t>(data[0]) |
               (static_cast<uint32_t>(data[1]) << 8) |
               (static_cast<uint32_t>(data[2]) << 16) |
               (static_cast<uint32_t>(data[3]) << 24);
    }

    bool chunkIdIs(const uint8_t *data, const char *id)
    {
        return data[0] == static_cast<uint8_t>(id[0]) &&
               data[1] == static_cast<uint8_t>(id[1]) &&
               data[2] == static_cast<uint8_t>(id[2]) &&
               data[3] == static_cast<uint8_t>(id[3]);
    }

    bool readExact(File &file, uint8_t *buffer, size_t size)
    {
        return file.read(buffer, size) == static_cast<int>(size);
    }

    bool parseAudioFilename(const char *name, uint8_t &id)
    {
        const size_t length = strlen(name);
        if (length < 8 || length > 31 || name[0] < '0' || name[0] > '9' ||
            name[1] < '0' || name[1] > '9' || name[2] != '_' ||
            strcmp(name + length - 4, ".wav") != 0) {
            return false;
        }

        id = static_cast<uint8_t>((name[0] - '0') * 10 + (name[1] - '0'));
        if (id == 0 || id > AUDIO_FILE_CAPACITY) return false;

        for (size_t i = 3; i < length - 4; i++) {
            const char ch = name[i];
            if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
                  ch == '_' || ch == '-')) {
                return false;
            }
        }
        return length > 7;
    }

    bool findPcmData(File &file, uint32_t &dataOffset, uint32_t &dataSize)
    {
        uint8_t header[12];
        const uint32_t fileSize = file.size();
        if (fileSize < sizeof(header) || !file.seek(0) || !readExact(file, header, sizeof(header)) ||
            !chunkIdIs(header, "RIFF") || !chunkIdIs(header + 8, "WAVE")) {
            return false;
        }

        const uint32_t riffSize = readLe32(header + 4);
        if (riffSize < 4 || riffSize > fileSize - 8) return false;
        const uint32_t riffEnd = riffSize + 8;
        uint32_t offset = 12;
        bool validFormat = false;
        bool foundData = false;

        while (offset <= riffEnd && riffEnd - offset >= 8) {
            uint8_t chunkHeader[8];
            if (!file.seek(offset) || !readExact(file, chunkHeader, sizeof(chunkHeader))) return false;

            const uint32_t chunkSize = readLe32(chunkHeader + 4);
            const uint32_t chunkDataOffset = offset + 8;
            if (chunkSize > riffEnd - chunkDataOffset) return false;

            if (chunkIdIs(chunkHeader, "fmt ")) {
                uint8_t format[16];
                if (chunkSize < sizeof(format) || !file.seek(chunkDataOffset) ||
                    !readExact(file, format, sizeof(format))) {
                    return false;
                }
                validFormat = readLe16(format) == 1 &&
                              readLe16(format + 2) == 1 &&
                              readLe32(format + 4) == 16000 &&
                              readLe32(format + 8) == 32000 &&
                              readLe16(format + 12) == 2 &&
                              readLe16(format + 14) == 16;
            } else if (chunkIdIs(chunkHeader, "data")) {
                if (foundData || chunkSize == 0 || (chunkSize & 1U)) return false;
                dataOffset = chunkDataOffset;
                dataSize = chunkSize;
                foundData = true;
            }

            const uint32_t paddedChunkSize = chunkSize + (chunkSize & 1U);
            if (paddedChunkSize > riffEnd - chunkDataOffset) return false;
            offset = chunkDataOffset + paddedChunkSize;
        }

        return validFormat && foundData;
    }

    void updateAudioPlayback()
    {
        if (!audioTestReading || !Sound::isSdStreamActive()) {
            if (audioTestReading) audioTestFile.close();
            audioTestReading = false;
            return;
        }

        if (pendingSampleOffset < pendingSamples) {
            pendingSampleOffset += Sound::pushSdStreamSamples(
                pcmSamples + pendingSampleOffset,
                pendingSamples - pendingSampleOffset);
            if (pendingSampleOffset < pendingSamples) return;
            pendingSamples = 0;
            pendingSampleOffset = 0;
        }

        if (audioDataRemaining == 0) {
            Sound::finishSdStream();
            audioTestReading = false;
            audioTestFile.close();
            return;
        }

        const size_t freeSamples = Sound::sdStreamFreeSamples();
        if (freeSamples == 0) return;
        const size_t bytesToRead = std::min(
            std::min(freeSamples, PCM_READ_SAMPLES),
            static_cast<size_t>(audioDataRemaining / 2)) * 2;
        const int bytesRead = audioTestFile.read(pcmReadBytes, bytesToRead);
        if (bytesRead <= 0 || (bytesRead & 1)) {
            Sound::finishSdStream();
            audioTestReading = false;
            audioTestFile.close();
            return;
        }

        const size_t samplesRead = static_cast<size_t>(bytesRead) / 2;
        for (size_t i = 0; i < samplesRead; i++) {
            pcmSamples[i] = static_cast<int16_t>(readLe16(pcmReadBytes + i * 2));
        }
        audioDataRemaining -= static_cast<uint32_t>(bytesRead);
        pendingSamples = samplesRead;
    }
}

namespace SDCard
{
    bool init()
    {
        if (initialized) return true;

        pinMode(Pins::SD::CS, OUTPUT);
        digitalWrite(Pins::SD::CS, HIGH);
        initialized = SD.begin(Pins::SD::CS, SD_SCK_MHZ(20), SPI);
        return initialized;
    }

    bool isReady()
    {
        return initialized;
    }

    bool scanAudioFiles()
    {
        sAudioFileCount = 0;
        if (!initialized) return false;

        File root = SD.open("/");
        if (!root || !root.isDirectory()) {
            if (root) root.close();
            return false;
        }

        File entry = root.openNextFile();
        while (entry && sAudioFileCount < AUDIO_FILE_CAPACITY) {
            if (!entry.isDirectory()) {
                uint8_t id = 0;
                const char *name = entry.name();
                if (name != nullptr && parseAudioFilename(name, id)) {
                    AudioFileInfo &info = audioFiles[sAudioFileCount++];
                    info.id = id;
                    strncpy(info.name, name, sizeof(info.name) - 1);
                    info.name[sizeof(info.name) - 1] = '\0';
                    info.size = entry.size();
                }
            }
            entry.close();
            entry = root.openNextFile();
        }
        root.close();
        return true;
    }

    uint8_t audioFileCount()
    {
        return sAudioFileCount;
    }

    bool audioFileInfo(uint8_t index, AudioFileInfo &info)
    {
        if (index >= sAudioFileCount) return false;
        info = audioFiles[index];
        return true;
    }

    PlayResult playAudioFile(uint8_t id)
    {
        if (!initialized) return PlayResult::NotReady;
        if (audioTestReading || Sound::isSdStreamActive()) return PlayResult::Busy;
        if (!scanAudioFiles()) return PlayResult::NotReady;

        const AudioFileInfo *selected = nullptr;
        for (uint8_t index = 0; index < sAudioFileCount; index++) {
            if (audioFiles[index].id != id) continue;
            if (selected != nullptr) return PlayResult::DuplicateId;
            selected = &audioFiles[index];
        }
        if (selected == nullptr) return PlayResult::NotFound;

        audioTestFile = SD.open(selected->name, FILE_READ);
        if (!audioTestFile) return PlayResult::NotFound;

        uint32_t dataOffset = 0;
        uint32_t dataSize = 0;
        if (!findPcmData(audioTestFile, dataOffset, dataSize)) {
            audioTestFile.close();
            return PlayResult::InvalidWav;
        }
        if (!audioTestFile.seek(dataOffset) || !Sound::beginSdStream()) {
            audioTestFile.close();
            return PlayResult::Busy;
        }

        audioDataRemaining = dataSize;
        pendingSamples = 0;
        pendingSampleOffset = 0;
        audioTestReading = true;
        return PlayResult::Started;
    }

    void update()
    {
        // Handshake de acces exclusiv (upload/verificare update): terminam
        // chunk-ul curent, oprim stream-ul audio si confirmam; cat timp
        // cererea e activa nu mai atingem cardul.
        if (sExclusiveRequested.load(std::memory_order_acquire)) {
            if (!sExclusiveAck.load(std::memory_order_relaxed)) {
                abortAudioStream();
                sExclusiveAck.store(true, std::memory_order_release);
            }
            return;
        }
        sExclusiveAck.store(false, std::memory_order_relaxed);
        updateAudioPlayback();
    }

    void abortAudioStream()
    {
        if (audioTestReading) {
            audioTestReading = false;
            audioTestFile.close();
            Sound::finishSdStream();
        }
        audioDataRemaining = 0;
        pendingSamples = 0;
        pendingSampleOffset = 0;
    }

    bool acquireExclusive(uint32_t timeoutMs)
    {
        if (!initialized) return false;
        sExclusiveAck.store(false, std::memory_order_relaxed);
        sExclusiveRequested.store(true, std::memory_order_release);
        const uint32_t deadline = millis() + timeoutMs;
        while (!sExclusiveAck.load(std::memory_order_acquire)) {
            if (static_cast<int32_t>(millis() - deadline) > 0) {
                sExclusiveRequested.store(false, std::memory_order_release);
                return false;
            }
            delay(1);
        }
        return true;
    }

    void releaseExclusive()
    {
        sExclusiveRequested.store(false, std::memory_order_release);
        sExclusiveAck.store(false, std::memory_order_relaxed);
    }

    bool isExclusiveRequested()
    {
        return sExclusiveRequested.load(std::memory_order_acquire);
    }
}