#ifndef FRAME_DECODER_H
#define FRAME_DECODER_H

#include <cstddef>
#include <cstdint>

class FrameDecoder {
public:
    enum class Event : uint8_t { Ignored, Started, Resynchronized, Byte, Complete };

    FrameDecoder(char *storage, size_t capacity);
    Event feed(char value, char startMarker, char endMarker, uint32_t now);
    bool checkTimeout(uint32_t now, uint32_t timeoutMs);
    bool hasFrame() const;
    bool overflowed() const;
    const char *frame() const;
    void reset();
    void consumeFrame();
    bool inProgress() const;

private:
    char *mStorage;
    size_t mCapacity;
    size_t mLength;
    uint32_t mStartedAt;
    bool mInProgress;
    bool mHasFrame;
    bool mOverflowed;
};

#endif