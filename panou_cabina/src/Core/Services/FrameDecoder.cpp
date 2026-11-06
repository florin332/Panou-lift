#include "FrameDecoder.h"

FrameDecoder::FrameDecoder(char *storage, size_t capacity)
    : mStorage(storage), mCapacity(capacity), mLength(0), mStartedAt(0),
      mInProgress(false), mHasFrame(false), mOverflowed(false) {
    if (mCapacity > 0) mStorage[0] = '\0';
}

FrameDecoder::Event FrameDecoder::feed(char value, char startMarker, char endMarker, uint32_t now) {
    if (mHasFrame) return Event::Ignored;

    if (value == startMarker) {
        const bool resynchronizing = mInProgress;
        mInProgress = true;
        mLength = 0;
        mStartedAt = now;
        mOverflowed = false;
        if (mCapacity > 0) mStorage[0] = '\0';
        return resynchronizing ? Event::Resynchronized : Event::Started;
    }

    if (!mInProgress) return Event::Ignored;
    if (value == endMarker) {
        if (mCapacity > 0) mStorage[mLength] = '\0';
        mInProgress = false;
        mHasFrame = true;
        return Event::Complete;
    }

    if (mLength + 1 < mCapacity) {
        mStorage[mLength++] = value;
        mStorage[mLength] = '\0';
    } else {
        mOverflowed = true;
    }
    return Event::Byte;
}

bool FrameDecoder::checkTimeout(uint32_t now, uint32_t timeoutMs) {
    if (!mInProgress || static_cast<uint32_t>(now - mStartedAt) <= timeoutMs) return false;
    mInProgress = false;
    mLength = 0;
    mOverflowed = false;
    if (mCapacity > 0) mStorage[0] = '\0';
    return true;
}

bool FrameDecoder::hasFrame() const { return mHasFrame; }
bool FrameDecoder::overflowed() const { return mOverflowed; }
const char *FrameDecoder::frame() const { return mStorage; }

void FrameDecoder::reset() {
    mLength = 0;
    mStartedAt = 0;
    mInProgress = false;
    mHasFrame = false;
    mOverflowed = false;
    if (mCapacity > 0) mStorage[0] = '\0';
}

void FrameDecoder::consumeFrame() {
    mHasFrame = false;
    mLength = 0;
    mOverflowed = false;
    if (mCapacity > 0) mStorage[0] = '\0';
}

bool FrameDecoder::inProgress() const { return mInProgress; }