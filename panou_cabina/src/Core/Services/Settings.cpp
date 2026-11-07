// src/Core/Services/Settings.cpp
// Persistență în zona de flash protejată (EEPROM emulat, ultimul sector).

#include "Settings.h"
#include "Config.h"

#include <EEPROM.h>
#include <cstddef>
#include <cmath>

namespace
{
    constexpr uint32_t SETTINGS_MAGIC   = 0x4C494654UL;  // "LIFT"
    constexpr uint16_t SETTINGS_VERSION = 1;
    constexpr size_t   EEPROM_SIZE      = 96;
    constexpr size_t   NETWORK_OFFSET   = 64;
    constexpr uint32_t NETWORK_MAGIC    = 0x4E455457UL;  // "NETW"
    constexpr uint16_t NETWORK_VERSION  = 1;

    struct PersistedData
    {
        uint32_t magic;
        uint16_t version;
        uint16_t liftNumber;
        float    audioGain;
        uint32_t crc32;
    };

    struct PersistedNetworkSettings
    {
        uint32_t magic;
        uint16_t version;
        uint8_t dhcp;
        uint8_t reserved;
        uint8_t ip[4];
        uint8_t subnet[4];
        uint8_t gateway[4];
        uint8_t dns[4];
        uint32_t crc32;
    };

    static_assert(sizeof(PersistedData) <= 32, "Settings overlap reset counter storage");
    static_assert(sizeof(PersistedNetworkSettings) == 28, "Network settings EEPROM layout changed");
    static_assert(NETWORK_OFFSET + sizeof(PersistedNetworkSettings) <= EEPROM_SIZE,
                  "Network settings exceed EEPROM storage");

    PersistedData data{};
    PersistedNetworkSettings networkData{};
    bool loaded = false;

    uint32_t crc32(const uint8_t *bytes, size_t length)
    {
        uint32_t crc = 0xFFFFFFFFUL;
        for (size_t i = 0; i < length; i++)
        {
            crc ^= bytes[i];
            for (uint8_t bit = 0; bit < 8; bit++)
                crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
        }
        return ~crc;
    }

    uint32_t computeCrc(const PersistedData &d)
    {
        return crc32(reinterpret_cast<const uint8_t *>(&d),
                     offsetof(PersistedData, crc32));
    }

    uint32_t computeNetworkCrc(const PersistedNetworkSettings &settings)
    {
        return crc32(reinterpret_cast<const uint8_t *>(&settings),
                     offsetof(PersistedNetworkSettings, crc32));
    }

    uint32_t toAddress(const uint8_t address[4])
    {
        return (static_cast<uint32_t>(address[0]) << 24) |
               (static_cast<uint32_t>(address[1]) << 16) |
               (static_cast<uint32_t>(address[2]) << 8) |
               static_cast<uint32_t>(address[3]);
    }

    bool networkFieldsAreValid(const PersistedNetworkSettings &settings)
    {
        if (settings.dhcp > 1) return false;
        if (settings.dhcp == 1) return true;

        const uint32_t ip = toAddress(settings.ip);
        const uint32_t subnet = toAddress(settings.subnet);
        const uint32_t gateway = toAddress(settings.gateway);
        const uint32_t dns = toAddress(settings.dns);
        const uint32_t inverseSubnet = ~subnet;
        if (ip == 0 || ip == UINT32_MAX || settings.ip[0] == 0 || settings.ip[0] == 127 || settings.ip[0] >= 224 ||
            subnet == 0 || subnet == UINT32_MAX || (inverseSubnet & (inverseSubnet + 1U)) != 0 ||
            dns == 0 || dns == UINT32_MAX || (ip & subnet) != (gateway & subnet) || ip == gateway)
            return false;

        const uint32_t ipHost = ip & inverseSubnet;
        const uint32_t gatewayHost = gateway & inverseSubnet;
        return ipHost != 0 && ipHost != inverseSubnet &&
               gatewayHost != 0 && gatewayHost != inverseSubnet;
    }

    bool networkDataIsValid(const PersistedNetworkSettings &settings)
    {
        return settings.magic == NETWORK_MAGIC && settings.version == NETWORK_VERSION &&
               settings.reserved == 0 && networkFieldsAreValid(settings) &&
               computeNetworkCrc(settings) == settings.crc32;
    }

    void loadNetworkDefaults()
    {
        networkData = {};
        networkData.magic = NETWORK_MAGIC;
        networkData.version = NETWORK_VERSION;
        networkData.dhcp = 1;
    }

    bool saveNetworkSettings()
    {
        networkData.magic = NETWORK_MAGIC;
        networkData.version = NETWORK_VERSION;
        networkData.reserved = 0;
        networkData.crc32 = computeNetworkCrc(networkData);
        EEPROM.put(NETWORK_OFFSET, networkData);
        return EEPROM.commit();
    }

    bool gainIsValid(float gain)
    {
        return !std::isnan(gain) &&
               gain >= Config::Audio::MIN_GAIN &&
               gain <= Config::Audio::MAX_GAIN;
    }

    float clampGain(float gain)
    {
        if (std::isnan(gain) || gain < Config::Audio::MIN_GAIN)
            return Config::Audio::MIN_GAIN;
        if (gain > Config::Audio::MAX_GAIN)
            return Config::Audio::MAX_GAIN;
        return gain;
    }

    void loadDefaults()
    {
        data.magic      = SETTINGS_MAGIC;
        data.version    = SETTINGS_VERSION;
        data.liftNumber = Config::Hardware::LIFT_NUMBER_DEFAULT;
        data.audioGain  = Config::Audio::DEFAULT_GAIN;
    }

    bool dataIsValid(const PersistedData &d)
    {
        return d.magic == SETTINGS_MAGIC &&
               d.version == SETTINGS_VERSION &&
               d.liftNumber >= 1 &&
               gainIsValid(d.audioGain) &&
               computeCrc(d) == d.crc32;
    }
}

namespace Settings
{
    void init()
    {
        if (loaded)
            return;

        EEPROM.begin(EEPROM_SIZE);
        EEPROM.get(0, data);

        if (!dataIsValid(data))
        {
            loadDefaults();
            save();
        }

        EEPROM.get(NETWORK_OFFSET, networkData);
        if (!networkDataIsValid(networkData))
        {
            loadNetworkDefaults();
            saveNetworkSettings();
        }

        loaded = true;
    }

    bool save()
    {
        data.magic   = SETTINGS_MAGIC;
        data.version = SETTINGS_VERSION;
        data.crc32   = computeCrc(data);
        EEPROM.put(0, data);
        return EEPROM.commit();
    }

    uint16_t liftNumber()
    {
        return data.liftNumber;
    }

    bool setLiftNumber(uint16_t number)
    {
        if (number < 1)
            return false;

        data.liftNumber = number;
        return save();
    }

    float audioGain()
    {
        return data.audioGain;
    }

    bool setAudioGain(float gain)
    {
        if (!gainIsValid(gain))
            return false;

        data.audioGain = clampGain(gain);
        return save();
    }

    bool resetDefaults()
    {
        loadDefaults();
        return save();
    }

    NetworkSettings networkSettings()
    {
        NetworkSettings settings{};
        settings.dhcp = networkData.dhcp;
        for (uint8_t i = 0; i < 4; i++) {
            settings.ip[i] = networkData.ip[i];
            settings.subnet[i] = networkData.subnet[i];
            settings.gateway[i] = networkData.gateway[i];
            settings.dns[i] = networkData.dns[i];
        }
        return settings;
    }

    bool setNetworkSettings(const NetworkSettings &settings)
    {
        PersistedNetworkSettings candidate{};
        candidate.magic = NETWORK_MAGIC;
        candidate.version = NETWORK_VERSION;
        candidate.dhcp = settings.dhcp;
        for (uint8_t i = 0; i < 4; i++) {
            candidate.ip[i] = settings.ip[i];
            candidate.subnet[i] = settings.subnet[i];
            candidate.gateway[i] = settings.gateway[i];
            candidate.dns[i] = settings.dns[i];
        }
        if (!networkFieldsAreValid(candidate)) return false;

        networkData = candidate;
        return saveNetworkSettings();
    }
}
