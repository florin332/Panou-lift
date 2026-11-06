#include "EncoderProtocol.h"
#include <cstdlib>
#include <cstring>

namespace EncoderProtocol {
namespace {

constexpr uint8_t RX_FIELDS = 5;
constexpr char CRC_SEPARATOR = '*';

uint8_t crc8Ccitt(const char *data, size_t length) {
    uint8_t crc = 0;
    for (size_t index = 0; index < length; ++index) {
        crc ^= static_cast<uint8_t>(data[index]);
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07) : static_cast<uint8_t>(crc << 1);
        }
    }
    return crc;
}

bool hexByteFromChars(char high, char low, uint8_t &out) {
    auto nibble = [](char value) -> int8_t {
        if (value >= '0' && value <= '9') return value - '0';
        if (value >= 'A' && value <= 'F') return value - 'A' + 10;
        if (value >= 'a' && value <= 'f') return value - 'a' + 10;
        return -1;
    };

    const int8_t highNibble = nibble(high);
    const int8_t lowNibble = nibble(low);
    if (highNibble < 0 || lowNibble < 0) return false;
    out = static_cast<uint8_t>((highNibble << 4) | lowNibble);
    return true;
}

bool isNumeric(const char *value) {
    if (value == nullptr || *value == '\0') return false;
    if (*value == '-') value++;
    while (*value) {
        if (*value < '0' || *value > '9') return false;
        value++;
    }
    return true;
}

}

FrameResult validateFrame(const char *frame, char *payload, size_t payloadCapacity) {
    if (frame == nullptr || payload == nullptr || payloadCapacity == 0) return FrameResult::FormatError;

    const char *separator = strchr(frame, CRC_SEPARATOR);
    if (separator == nullptr || separator[1] == '\0' || separator[2] == '\0' || separator[3] != '\0') {
        return FrameResult::FormatError;
    }

    uint8_t receivedCrc;
    if (!hexByteFromChars(separator[1], separator[2], receivedCrc)) return FrameResult::FormatError;
    const size_t payloadLength = static_cast<size_t>(separator - frame);
    if (crc8Ccitt(frame, payloadLength) != receivedCrc) return FrameResult::CrcError;
    if (payloadLength >= payloadCapacity) return FrameResult::FormatError;
    memcpy(payload, frame, payloadLength);
    payload[payloadLength] = '\0';
    return FrameResult::Valid;
}

ParseResult parsePayload(const char *payload, uint8_t floorCount, LiftState &result) {
    if (payload == nullptr) return ParseResult::FormatError;

    uint8_t commaCount = 0;
    for (const char *value = payload; *value; ++value) {
        if (*value == ',') commaCount++;
    }
    if (commaCount != RX_FIELDS - 1) return ParseResult::FormatError;

    char localBuffer[40];
    strncpy(localBuffer, payload, sizeof(localBuffer) - 1);
    localBuffer[sizeof(localBuffer) - 1] = '\0';

    char *fields[RX_FIELDS] = {localBuffer};
    for (uint8_t index = 1; index < RX_FIELDS; ++index) {
        char *separator = strchr(fields[index - 1], ',');
        if (separator == nullptr) return ParseResult::FormatError;
        *separator = '\0';
        fields[index] = separator + 1;
    }
    if (strchr(fields[RX_FIELDS - 1], ',') != nullptr) return ParseResult::FormatError;

    for (uint8_t index = 0; index < RX_FIELDS; ++index) {
        if (!isNumeric(fields[index])) return ParseResult::FormatError;
    }

    const int rawPosition = atoi(fields[0]);
    if (rawPosition < 0 || rawPosition >= floorCount) return ParseResult::DataError;
    result.pos = static_cast<uint8_t>(rawPosition);

    const int rawDestination = atoi(fields[1]);
    if (rawDestination < 0 || rawDestination >= floorCount) return ParseResult::DataError;
    result.etd = static_cast<uint8_t>(rawDestination);

    const int rawOccupancy = atoi(fields[2]);
    if (rawOccupancy != 0 && rawOccupancy != 1) return ParseResult::DataError;
    result.ocp = rawOccupancy == 1 ? Occupancy::Busy : Occupancy::Free;

    const int rawDirection = atoi(fields[3]);
    if (rawDirection < 0 || rawDirection > 2) return ParseResult::DataError;
    result.sj = rawDirection == 1 ? Direction::Up : rawDirection == 2 ? Direction::Down : Direction::Idle;

    const int rawService = atoi(fields[4]);
    if (rawService != 1 && rawService != 2 && rawService != 3 && rawService != 5) return ParseResult::DataError;
    if (rawService == 1) result.svc = ServiceState::Fault;
    else if (rawService == 2) result.svc = ServiceState::Revision;
    else if (rawService == 3) result.svc = ServiceState::Missing;
    else result.svc = ServiceState::Normal;

    return ParseResult::Valid;
}

}