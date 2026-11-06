#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <cstddef>
#include <cstdint>

namespace Transport {

enum class Channel : uint8_t { EncoderRx, DeviceTx, Service };

class IByteTransport {
public:
    virtual ~IByteTransport() = default;
    virtual int available(Channel channel) = 0;
    virtual int read(Channel channel) = 0;
    virtual size_t write(Channel channel, const uint8_t *data, size_t length) = 0;
    virtual bool connected(Channel channel) const = 0;
};

IByteTransport &instance();
void init();
void poll();

}

#endif