// Drivers/Sound.cpp
// Driver audio pentru Marble Pico + MAX98357A
// ESP8266Audio / WAV / I2S

#include "Sound.h"
#include "Alerts.h"

#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>

namespace
{
    constexpr int I2S_BCLK  = 8;
    constexpr int I2S_LRCLK = 9;
    constexpr int I2S_DATA  = 10;

    constexpr int AUDIO_SAMPLE_RATE = 16000;
    constexpr float AUDIO_GAIN = 0.3f;

    class PersistentAudioOutputI2S : public AudioOutputI2S
    {
    public:
        bool stop() override
        {
            return true;
        }
    };

    AudioGeneratorWAV *wav = nullptr;
    AudioFileSourcePROGMEM *file = nullptr;
    PersistentAudioOutputI2S *out = nullptr;

    bool initialized = false;

    void stopCurrentSound()
    {
        if (wav != nullptr)
        {
            if (wav->isRunning())
                wav->stop();

            delete wav;
            wav = nullptr;
        }

        if (file != nullptr)
        {
            delete file;
            file = nullptr;
        }
    }

    void playWav(const uint8_t *data, uint32_t length)
    {
        if (!initialized || data == nullptr || length == 0)
            return;

        stopCurrentSound();

        // Drain any stale frames and force the DMA pipeline onto silence
        // before queueing the new stream.
        out->flush();

        file = new AudioFileSourcePROGMEM(data, length);

        if (file == nullptr)
            return;

        wav = new AudioGeneratorWAV();

        if (wav == nullptr)
        {
            delete file;
            file = nullptr;
            return;
        }

        if (!wav->begin(file, out))
        {
            delete wav;
            wav = nullptr;

            delete file;
            file = nullptr;
        }
    }
}

namespace Sound
{
    void init()
    {
        if (initialized)
            return;

        out = new PersistentAudioOutputI2S();

        if (out == nullptr)
            return;

        out->SetPinout(
            I2S_BCLK,
            I2S_LRCLK,
            I2S_DATA
        );

        out->SetRate(AUDIO_SAMPLE_RATE);
        out->SetGain(AUDIO_GAIN);

        if (!out->begin())
        {
            delete out;
            out = nullptr;
            return;
        }

        initialized = true;
    }

    void update(const SharedPanel &localPanel)
    {
        (void)localPanel;

        if (!initialized || wav == nullptr)
            return;

        if (wav->isRunning())
        {
            if (!wav->loop())
            {
                wav->stop();
            }
        }
    }

    void alarm()
    {
        playWav(
            alarm_wav,
            alarm_wav_len
        );
    }

    void overload()
    {
        playWav(
            overload_wav,
            overload_wav_len
        );
    }

    void liftStopped()
    {
        playWav(
            stop_wav,
            stop_wav_len
        );
    }

    void stop()
    {
        stopCurrentSound();
    }

    bool isPlaying()
    {
        return wav != nullptr && wav->isRunning();
    }
}