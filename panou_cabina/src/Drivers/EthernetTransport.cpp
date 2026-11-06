#define USING_SPI2 true

#include "Transport.h"
#include "Pins.h"
#include "Config.h"
#include <Arduino.h>
#include <Ethernet_Generic.h>
#include <SPI.h>
#include <atomic>

SPIClass *pCUR_SPI = &SPI1;

namespace Transport {
namespace {

constexpr uint16_t QUEUE_SIZE = 256;
constexpr uint16_t QUEUE_MASK = QUEUE_SIZE - 1;
constexpr uint32_t NETWORK_RETRY_MS = 5000;

class ByteQueue {
public:
    size_t push(const uint8_t *data, size_t length) {
        size_t written = 0;
        while (written < length) {
            const uint16_t head = mHead.load(std::memory_order_relaxed);
            const uint16_t next = static_cast<uint16_t>((head + 1) & QUEUE_MASK);
            if (next == mTail.load(std::memory_order_acquire)) break;
            mData[head] = data[written++];
            mHead.store(next, std::memory_order_release);
        }
        return written;
    }

    int pop() {
        const uint16_t tail = mTail.load(std::memory_order_relaxed);
        if (tail == mHead.load(std::memory_order_acquire)) return -1;
        const uint8_t value = mData[tail];
        mTail.store(static_cast<uint16_t>((tail + 1) & QUEUE_MASK), std::memory_order_release);
        return value;
    }

    int available() const {
        const uint16_t head = mHead.load(std::memory_order_acquire);
        const uint16_t tail = mTail.load(std::memory_order_acquire);
        return (head - tail) & QUEUE_MASK;
    }

private:
    uint8_t mData[QUEUE_SIZE] = {};
    std::atomic<uint16_t> mHead{0};
    std::atomic<uint16_t> mTail{0};
};

uint8_t sMacAddress[] = {
    Config::Ethernet::MAC[0], Config::Ethernet::MAC[1], Config::Ethernet::MAC[2],
    Config::Ethernet::MAC[3], Config::Ethernet::MAC[4], Config::Ethernet::MAC[5]
};

EthernetServer sEncoderServer(Config::Ethernet::ENCODER_PORT);
EthernetServer sDeviceServer(Config::Ethernet::DEVICE_PORT);
EthernetServer sServiceServer(Config::Ethernet::SERVICE_PORT);
EthernetClient sEncoderClient;
EthernetClient sDeviceClient;
EthernetClient sServiceClient;
ByteQueue sEncoderRx;
ByteQueue sDeviceTx;
ByteQueue sServiceTx;
bool sNetworkReady = false;
uint32_t sLastNetworkAttempt = 0;

struct TxCursor {
    uint8_t bytes[64];
    size_t length = 0;
    size_t offset = 0;
};

TxCursor sDevicePending;
TxCursor sServicePending;

void acceptClient(EthernetServer &server, EthernetClient &client) {
    EthernetClient incoming = server.accept();
    if (!incoming) return;
    if (client) client.stop();
    client = incoming;
}

void updateEncoderRx() {
    while (sEncoderClient && sEncoderClient.available() > 0) {
        const int value = sEncoderClient.read();
        if (value < 0) break;
        const uint8_t byteValue = static_cast<uint8_t>(value);
        if (sEncoderRx.push(&byteValue, 1) == 0) break;
    }
}

void flushTx(EthernetClient &client, ByteQueue &queue, TxCursor &pending) {
    if (!client || !client.connected()) return;
    if (pending.offset == pending.length) {
        pending.offset = 0;
        pending.length = 0;
        while (pending.length < sizeof(pending.bytes)) {
            const int value = queue.pop();
            if (value < 0) break;
            pending.bytes[pending.length++] = static_cast<uint8_t>(value);
        }
    }
    if (pending.length == 0) return;

    const int writable = client.availableForWrite();
    if (writable <= 0) return;
    const size_t remaining = pending.length - pending.offset;
    const size_t count = remaining < static_cast<size_t>(writable) ? remaining : static_cast<size_t>(writable);
        client.write(pending.bytes + pending.offset, count);
        pending.offset += count;
    if (pending.offset == pending.length) pending.length = pending.offset = 0;
}

class W5500Transport final : public IByteTransport {
public:
    int available(Channel channel) override {
        if (channel == Channel::EncoderRx) return sEncoderRx.available();
        if (channel == Channel::Service && sServiceClient) return sServiceClient.available();
        return 0;
    }

    int read(Channel channel) override {
        if (channel == Channel::EncoderRx) return sEncoderRx.pop();
        if (channel == Channel::Service && sServiceClient) return sServiceClient.read();
        return -1;
    }

    size_t write(Channel channel, const uint8_t *data, size_t length) override {
        if (data == nullptr) return 0;
        if (channel == Channel::DeviceTx) return sDeviceTx.push(data, length);
        if (channel == Channel::Service) return sServiceTx.push(data, length);
        return 0;
    }

    bool connected(Channel channel) const override {
        if (channel == Channel::EncoderRx) return sEncoderClient && sEncoderClient.connected();
        if (channel == Channel::DeviceTx) return sDeviceClient && sDeviceClient.connected();
        return channel == Channel::Service && sServiceClient && sServiceClient.connected();
    }
};

W5500Transport sTransport;

}

IByteTransport &instance() { return sTransport; }

void init() {
    pinMode(Pins::Ethernet::CS, OUTPUT);
    digitalWrite(Pins::Ethernet::CS, HIGH);
    SPI1.setRX(Pins::Ethernet::MISO);
    SPI1.setTX(Pins::Ethernet::MOSI);
    SPI1.setSCK(Pins::Ethernet::SCK);
    SPI1.begin();
    Ethernet.init(Pins::Ethernet::CS);
    sLastNetworkAttempt = millis() - NETWORK_RETRY_MS;
    poll();
}

static void startNetwork() {
    sLastNetworkAttempt = millis();
    sNetworkReady = Ethernet.begin(sMacAddress, &SPI1, Config::Ethernet::DHCP_TIMEOUT_MS,
                                   Config::Ethernet::DHCP_RESPONSE_TIMEOUT_MS) != 0;
    if (!sNetworkReady) return;
    sEncoderServer.begin();
    sDeviceServer.begin();
    sServiceServer.begin();
}

void poll() {
    if (!sNetworkReady) {
        if (millis() - sLastNetworkAttempt >= NETWORK_RETRY_MS) startNetwork();
        return;
    }
    acceptClient(sEncoderServer, sEncoderClient);
    acceptClient(sDeviceServer, sDeviceClient);
    acceptClient(sServiceServer, sServiceClient);
    if (sEncoderClient && !sEncoderClient.connected()) sEncoderClient.stop();
    if (sDeviceClient && !sDeviceClient.connected()) sDeviceClient.stop();
    if (sServiceClient && !sServiceClient.connected()) sServiceClient.stop();
    updateEncoderRx();
    flushTx(sDeviceClient, sDeviceTx, sDevicePending);
    flushTx(sServiceClient, sServiceTx, sServicePending);
}

}