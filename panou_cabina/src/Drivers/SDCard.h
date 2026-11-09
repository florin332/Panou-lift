#ifndef SD_CARD_H
#define SD_CARD_H

#include <cstddef>
#include <cstdint>

namespace SDCard
{
    struct AudioFileInfo {
        uint8_t id;
        char name[32];
        uint32_t size;
    };

    enum class PlayResult : uint8_t {
        Started,
        NotReady,
        NotFound,
        DuplicateId,
        InvalidWav,
        Busy
    };

    bool init();
    bool isReady();
    bool scanAudioFiles();
    uint8_t audioFileCount();
    bool audioFileInfo(uint8_t index, AudioFileInfo &info);
    PlayResult playAudioFile(uint8_t id);
    void update();
}

#endif