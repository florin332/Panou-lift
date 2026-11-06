#ifndef ENCODER_PROTOCOL_H
#define ENCODER_PROTOCOL_H

#include "SharedPanel.h"
#include <cstddef>
#include <cstdint>

namespace EncoderProtocol {

enum class FrameResult : uint8_t { None, FormatError, CrcError, Valid };
enum class ParseResult : uint8_t { FormatError, DataError, Valid };

FrameResult validateFrame(const char *frame, char *payload, size_t payloadCapacity);
ParseResult parsePayload(const char *payload, uint8_t floorCount, LiftState &result);

}

#endif