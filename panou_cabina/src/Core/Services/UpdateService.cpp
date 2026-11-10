// Core/Services/UpdateService.cpp
// Update firmware prin SD: upload UF2 (WebUi) -> verificare locala (2 treceri:
// structura + SHA-256) -> instalare locala din SD pe Core 0, dupa oprirea
// gratuitoasa a Core 1. Fisierele raman pe SD pana la confirmarea boot-ului.

#include "UpdateService.h"

#include "BuildInfo.h"
#include "SDCard.h"
#include "ServiceMenu.h"
#include "Sound.h"

#include <Arduino.h>
#include <SD.h>
#include <hardware/flash.h>
#include <hardware/watchdog.h>
#include <pico/multicore.h>
#include <atomic>
#include <cctype>
#include <cstring>

namespace UpdateService {
namespace {

// ---------------------------------------------------------------------------
// Constante
// ---------------------------------------------------------------------------

constexpr char UPDATE_DIR[]   = "/UPDATE";
constexpr char FW_PATH[]      = "/UPDATE/FIRMWARE.UF2";
constexpr char META_PATH[]    = "/UPDATE/FIRMWARE.META";
constexpr char BOOTMARK_PATH[]= "/UPDATE/BOOT.PND";
constexpr char FAILMARK_PATH[]= "/UPDATE/FAILED.TXT";

constexpr uint32_t MANIFEST_MAGIC   = 0x5557464DUL;  // 'UFWM'
constexpr uint8_t  MANIFEST_VERSION = 1;

constexpr uint32_t UF2_MAGIC_START0  = 0x0A324655UL;
constexpr uint32_t UF2_MAGIC_START1  = 0x9E5D5157UL;
constexpr uint32_t UF2_MAGIC_END     = 0x0AB16F30UL;
constexpr uint32_t UF2_FLAG_FAMILYID = 0x00002000UL;
constexpr uint32_t UF2_FAMILY_RP2040 = 0xE48BFF56UL;
constexpr uint32_t UF2_PAYLOAD_SIZE  = 256;

constexpr uint32_t APP_REGION_BEGIN = XIP_BASE;  // 0x10000000
constexpr uint32_t APP_REGION_END   = XIP_BASE + PICO_FLASH_SIZE_BYTES - 4096;
constexpr uint32_t SHIM_REGION_END  = XIP_BASE + 0x3000;  // boot2 + OTA shim

constexpr size_t   VERIFY_BLOCKS_PER_SLICE = 8;    // 4 KB per poll
constexpr uint32_t QUIESCE_TIMEOUT_MS      = 2000;
constexpr uint32_t EXCLUSIVE_TIMEOUT_MS    = 800;

// ---------------------------------------------------------------------------
// SHA-256 (implementare compacta, fara dependinte)
// ---------------------------------------------------------------------------

class Sha256 {
public:
    void begin() {
        static const uint32_t H0[8] = {
            0x6a09e667UL, 0xbb67ae85UL, 0x3c6ef372UL, 0xa54ff53aUL,
            0x510e527fUL, 0x9b05688cUL, 0x1f83d9abUL, 0x5be0cd19UL
        };
        memcpy(mState, H0, sizeof(mState));
        mBitLength = 0;
        mDataLength = 0;
    }

    void update(const uint8_t* data, size_t length) {
        for (size_t i = 0; i < length; i++) {
            mData[mDataLength++] = data[i];
            if (mDataLength == 64) {
                transform();
                mBitLength += 512;
                mDataLength = 0;
            }
        }
    }

    void finish(uint8_t out[32]) {
        uint32_t i = mDataLength;
        mData[i++] = 0x80;
        if (i > 56) {
            while (i < 64) mData[i++] = 0;
            transform();
            i = 0;
        }
        while (i < 56) mData[i++] = 0;
        mBitLength += static_cast<uint64_t>(mDataLength) * 8;
        for (int shift = 56; shift >= 0; shift -= 8) {
            mData[i++] = static_cast<uint8_t>((mBitLength >> shift) & 0xFF);
        }
        transform();
        for (i = 0; i < 8; i++) {
            out[i * 4 + 0] = static_cast<uint8_t>((mState[i] >> 24) & 0xFF);
            out[i * 4 + 1] = static_cast<uint8_t>((mState[i] >> 16) & 0xFF);
            out[i * 4 + 2] = static_cast<uint8_t>((mState[i] >> 8) & 0xFF);
            out[i * 4 + 3] = static_cast<uint8_t>(mState[i] & 0xFF);
        }
    }

private:
    static uint32_t rotr(uint32_t value, uint32_t bits) {
        return (value >> bits) | (value << (32 - bits));
    }

    void transform() {
        static const uint32_t K[64] = {
            0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL,
            0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
            0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL,
            0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
            0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
            0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
            0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL,
            0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
            0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL,
            0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
            0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL,
            0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
            0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL,
            0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
            0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
            0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
        };
        uint32_t m[64];
        for (uint32_t i = 0, j = 0; i < 16; i++, j += 4) {
            m[i] = (static_cast<uint32_t>(mData[j]) << 24) |
                   (static_cast<uint32_t>(mData[j + 1]) << 16) |
                   (static_cast<uint32_t>(mData[j + 2]) << 8) |
                   (static_cast<uint32_t>(mData[j + 3]));
        }
        for (uint32_t i = 16; i < 64; i++) {
            const uint32_t s0 = rotr(m[i - 15], 7) ^ rotr(m[i - 15], 18) ^ (m[i - 15] >> 3);
            const uint32_t s1 = rotr(m[i - 2], 17) ^ rotr(m[i - 2], 19) ^ (m[i - 2] >> 10);
            m[i] = m[i - 16] + s0 + m[i - 7] + s1;
        }
        uint32_t a = mState[0], b = mState[1], c = mState[2], d = mState[3];
        uint32_t e = mState[4], f = mState[5], g = mState[6], h = mState[7];
        for (uint32_t i = 0; i < 64; i++) {
            const uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const uint32_t ch = (e & f) ^ (~e & g);
            const uint32_t t1 = h + s1 + ch + K[i] + m[i];
            const uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t t2 = s0 + maj;
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }
        mState[0] += a; mState[1] += b; mState[2] += c; mState[3] += d;
        mState[4] += e; mState[5] += f; mState[6] += g; mState[7] += h;
    }

    uint32_t mState[8];
    uint64_t mBitLength;
    uint8_t  mData[64];
    uint32_t mDataLength;
};

// ---------------------------------------------------------------------------
// Manifest (metadate verificare, persistent pe SD)
// ---------------------------------------------------------------------------

struct Manifest {
    uint32_t magic;
    uint8_t  version;
    char     fileName[24];
    uint32_t fileSize;
    uint32_t blockCount;
    uint32_t firstAddr;
    uint32_t lastAddrEnd;
    uint8_t  sha256[32];
    char     panelType;
    uint8_t  hwVariant;
    uint8_t  fwMajor;
    uint8_t  fwMinor;
    uint8_t  compat;       // 0 necunoscut, 1 ok, 2 mismatch
    uint32_t crc32;        // peste tot, mai putin acest camp
};

uint32_t crc32Update(uint32_t crc, const uint8_t* data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320UL : (crc >> 1);
        }
    }
    return crc;
}

uint32_t manifestCrc(const Manifest& manifest) {
    return ~crc32Update(0xFFFFFFFFUL, reinterpret_cast<const uint8_t*>(&manifest),
                        offsetof(Manifest, crc32));
}

// ---------------------------------------------------------------------------
// Stare interna
// ---------------------------------------------------------------------------

Phase sPhase = Phase::Idle;
char sError[48] = {};
char sFileName[33] = {};
uint32_t sTotalBytes = 0;
uint32_t sProgressBytes = 0;
File sFile;
bool sExclusiveHeld = false;
bool sCompatKnown = false;
bool sCompat = false;
uint8_t sFwMajor = 0;
uint8_t sFwMinor = 0;
bool sBootConfirmed = false;
bool sBootCheckDone = false;

Manifest sManifest;
bool sManifestValid = false;

// Verificare feliata
uint32_t sVerifyBlock = 0;
Sha256 sSha;

// Handshake Core 1
std::atomic<bool> sQuiesceRequested{false};
std::atomic<bool> sQuiescedAck{false};
uint32_t sQuiesceDeadline = 0;

void setError(const char* message) {
    snprintf(sError, sizeof(sError), "%s", message != nullptr ? message : "");
}

void fail(const char* message) {
    setError(message);
    sPhase = Phase::Failed;
    Serial.printf("[OTA] ESEC: %s\n", sError);
}

void clearError() {
    sError[0] = '\0';
}

uint32_t readLe32(const uint8_t* data) {
    return static_cast<uint32_t>(data[0]) |
           (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) |
           (static_cast<uint32_t>(data[3]) << 24);
}

// Cat timp detinem cardul, armam watchdog-ul: orice blocaj > 2 s pe
// Core 0 (SD, retea, audio) produce auto-reset in loc de panou mort.
bool acquireSd() {
    if (sExclusiveHeld) return true;
    if (!SDCard::acquireExclusive(EXCLUSIVE_TIMEOUT_MS)) return false;
    sExclusiveHeld = true;
    watchdog_enable(2000, true);
    return true;
}

void releaseSd() {
    if (!sExclusiveHeld) return;
    sExclusiveHeld = false;
    watchdog_disable();
    SDCard::releaseExclusive();
}

// Compatibilitate din numele fisierului: prefix "<tip>_<hh>" (ex. "C_01"),
// optional urmat de "_<major>.<minor>".
void parseCompatibility(const char* name) {
    sCompatKnown = false;
    sCompat = false;
    sFwMajor = 0;
    sFwMinor = 0;
    if (name == nullptr) return;
    const size_t length = strlen(name);
    if (length < 4 || !isalpha(static_cast<uint8_t>(name[0])) || name[1] != '_' ||
        !isdigit(static_cast<uint8_t>(name[2])) || !isdigit(static_cast<uint8_t>(name[3]))) {
        return;
    }
    sCompatKnown = true;
    const unsigned variant = static_cast<unsigned>((name[2] - '0') * 10 + (name[3] - '0'));
    sCompat = (toupper(static_cast<uint8_t>(name[0])) == BuildInfo::PanelType) &&
              (variant == BuildInfo::HardwareVariant);
    if (length >= 9 && name[4] == '_' && isdigit(static_cast<uint8_t>(name[5])) &&
        name[6] == '.' && isdigit(static_cast<uint8_t>(name[7])) &&
        isdigit(static_cast<uint8_t>(name[8]))) {
        sFwMajor = static_cast<uint8_t>(name[5] - '0');
        sFwMinor = static_cast<uint8_t>((name[7] - '0') * 10 + (name[8] - '0'));
    }
}

// Validarea unui bloc UF2; index = pozitia in fisier. Verificarile de
// secventa (adresa asteptata) se bazeaza pe prima adresa din imagine.
bool validateBlock(const uint8_t* block, uint32_t index, uint32_t totalBlocks,
                   uint32_t firstAddr, const char*& error) {
    if (readLe32(block) != UF2_MAGIC_START0 || readLe32(block + 4) != UF2_MAGIC_START1 ||
        readLe32(block + 508) != UF2_MAGIC_END) {
        error = "Bloc UF2 cu magic invalid";
        return false;
    }
    const uint32_t flags = readLe32(block + 8);
    if ((flags & UF2_FLAG_FAMILYID) == 0 || readLe32(block + 28) != UF2_FAMILY_RP2040) {
        error = "Imaginea nu este pentru RP2040";
        return false;
    }
    if (readLe32(block + 16) != UF2_PAYLOAD_SIZE) {
        error = "Payload UF2 diferit de 256 octeti";
        return false;
    }
    if (readLe32(block + 20) != index || readLe32(block + 24) != totalBlocks) {
        error = "Numerotare blocuri incoerenta";
        return false;
    }
    const uint32_t target = readLe32(block + 12);
    if ((target & 0xFF) != 0) {
        error = "Adresa nealiniata la 256 octeti";
        return false;
    }
    if (target != firstAddr + index * UF2_PAYLOAD_SIZE) {
        error = "Adrese neconsecutive sau duplicate";
        return false;
    }
    if (target < APP_REGION_BEGIN || target + UF2_PAYLOAD_SIZE > APP_REGION_END) {
        error = "Adresa in afara memoriei fizice";
        return false;
    }
    return true;
}

bool writeManifest() {
    sManifest.crc32 = manifestCrc(sManifest);
    SD.remove(META_PATH);
    File meta = SD.open(META_PATH, FILE_WRITE);
    if (!meta) return false;
    const size_t written = meta.write(reinterpret_cast<const uint8_t*>(&sManifest), sizeof(sManifest));
    meta.close();
    return written == sizeof(sManifest);
}

bool readManifest() {
    File meta = SD.open(META_PATH, FILE_READ);
    if (!meta) return false;
    const int read = meta.read(reinterpret_cast<uint8_t*>(&sManifest), sizeof(sManifest));
    meta.close();
    if (read != static_cast<int>(sizeof(sManifest))) return false;
    if (sManifest.magic != MANIFEST_MAGIC || sManifest.version != MANIFEST_VERSION) return false;
    return sManifest.crc32 == manifestCrc(sManifest);
}

// Hash complet al fisierului; folosit la re-verificarea din Apply.
bool hashFirmwareFile(uint8_t out[32]) {
    File file = SD.open(FW_PATH, FILE_READ);
    if (!file) return false;
    Sha256 sha;
    sha.begin();
    uint8_t chunk[1024];
    while (true) {
        const int read = file.read(chunk, sizeof(chunk));
        if (read < 0) { file.close(); return false; }
        if (read == 0) break;
        sha.update(chunk, static_cast<size_t>(read));
        watchdog_update();
    }
    file.close();
    sha.finish(out);
    return true;
}

// ---------------------------------------------------------------------------
// Instalare (Core 0 unic, Core 1 oprit). Nu returneaza: reboot la final.
// ---------------------------------------------------------------------------

void writeFailMark(const char* message) {
    SD.remove(FAILMARK_PATH);
    File mark = SD.open(FAILMARK_PATH, FILE_WRITE);
    if (mark) {
        mark.print(message != nullptr ? message : "eroare instalare");
        mark.close();
    }
}

bool flushPage(uint8_t* page, uint32_t pageBase, const char*& error) {
    if (pageBase < SHIM_REGION_END) {
        return true;  // boot2 + OTA shim raman neatinsi
    }
    const uint8_t* current = reinterpret_cast<const uint8_t*>(pageBase);
    if (memcmp(page, current, 4096) == 0) {
        return true;  // pagina identica: skip
    }
    noInterrupts();
    flash_range_erase(pageBase - XIP_BASE, 4096);
    flash_range_program(pageBase - XIP_BASE, page, 4096);
    interrupts();
    watchdog_update();
    if (memcmp(page, current, 4096) != 0) {
        error = "Verificare flash esuata dupa programare";
        return false;
    }
    return true;
}

__attribute__((noreturn))
void installFirmware() {
    sPhase = Phase::Installing;
    const char* error = nullptr;
    static uint8_t page[4096];
    uint32_t pageBase = 0;
    size_t pageFill = 0;

    File file = SD.open(FW_PATH, FILE_READ);
    if (!file) {
        Serial.println("[OTA] instalare: fisierul UF2 nu se deschide");
        writeFailMark("Fisierul UF2 nu se deschide la instalare");
        watchdog_reboot(0, 0, 200);
        while (true) tight_loop_contents();
    }

    uint8_t block[512];
    uint32_t index = 0;
    while (true) {
        const int read = file.read(block, sizeof(block));
        if (read == 0) break;
        if (read != static_cast<int>(sizeof(block)) ||
            !validateBlock(block, index, sManifest.blockCount, sManifest.firstAddr, error)) {
            if (error == nullptr) error = "Citire UF2 esuata la instalare";
            break;
        }
        const uint32_t target = readLe32(block + 12);
        const uint32_t targetPage = target & ~0xFFFUL;
        if (pageFill > 0 && targetPage != pageBase) {
            if (!flushPage(page, pageBase, error)) break;
            pageFill = 0;
        }
        if (pageFill == 0) {
            pageBase = targetPage;
            memset(page, 0xFF, sizeof(page));
        }
        memcpy(page + (target - pageBase), block + 32, UF2_PAYLOAD_SIZE);
        pageFill = (target - pageBase) + UF2_PAYLOAD_SIZE;
        index++;
        watchdog_update();
    }

    if (error == nullptr && pageFill > 0) {
        if (!flushPage(page, pageBase, error)) { /* error setat */ }
    }
    file.close();

    if (error != nullptr || index != sManifest.blockCount) {
        if (error == nullptr) error = "Numar de blocuri instalat incoerent";
        Serial.printf("[OTA] instalare ESUATA: %s\n", error);
        writeFailMark(error);
    } else {
        Serial.println("[OTA] instalare OK; reboot");
        SD.remove(BOOTMARK_PATH);
        File mark = SD.open(BOOTMARK_PATH, FILE_WRITE);
        if (mark) {
            mark.print("boot pending");
            mark.close();
        }
    }
    watchdog_reboot(0, 0, 200);
    while (true) tight_loop_contents();
}

void checkBootMarker() {
    if (sBootCheckDone || !SDCard::isReady()) return;
    sBootCheckDone = true;
    if (!acquireSd()) return;
    if (SD.exists(FAILMARK_PATH)) {
        File mark = SD.open(FAILMARK_PATH, FILE_READ);
        if (mark) {
            const int read = mark.read(reinterpret_cast<uint8_t*>(sError), sizeof(sError) - 1);
            if (read > 0) sError[read] = '\0';
            mark.close();
        }
        SD.remove(FAILMARK_PATH);
        sPhase = Phase::Failed;
    } else if (SD.exists(BOOTMARK_PATH)) {
        SD.remove(BOOTMARK_PATH);
        sBootConfirmed = true;
        // Firmware-ul nou a pornit: UF2-ul se pastreaza pe SD, starea
        // devine Completed; fisierul poate fi inlocuit la urmatorul upload.
        if (sPhase == Phase::Idle) sPhase = Phase::Completed;
    }
    releaseSd();
}

void verifySlice() {
    for (size_t i = 0; i < VERIFY_BLOCKS_PER_SLICE && sVerifyBlock < sManifest.blockCount; i++) {
        uint8_t block[512];
        const int read = sFile.read(block, sizeof(block));
        const char* error = nullptr;
        if (read != static_cast<int>(sizeof(block))) {
            fail("Citire UF2 esuata la verificare");
        } else if (!validateBlock(block, sVerifyBlock, sManifest.blockCount,
                                  sManifest.firstAddr, error)) {
            fail(error);
        } else {
            sSha.update(block, sizeof(block));
            sVerifyBlock++;
            sProgressBytes += sizeof(block);
        }
        if (sPhase != Phase::Verifying) {
            sFile.close();
            releaseSd();
            return;
        }
    }

    if (sVerifyBlock < sManifest.blockCount) return;

    sFile.close();
    sSha.finish(sManifest.sha256);
    sManifest.lastAddrEnd = sManifest.firstAddr + sManifest.blockCount * UF2_PAYLOAD_SIZE;
    if (!writeManifest()) {
        fail("Manifestul nu poate fi scris pe SD");
    } else {
        sManifestValid = true;
        sPhase = Phase::Verified;
        Serial.printf("[OTA] verificare OK: %lu blocuri\n",
                      static_cast<unsigned long>(sManifest.blockCount));
    }
    releaseSd();
}

} // namespace

// ---------------------------------------------------------------------------
// API public
// ---------------------------------------------------------------------------

void init() {
    sPhase = Phase::Idle;
    clearError();
}

void update() {
    if (sExclusiveHeld) watchdog_update();
    checkBootMarker();

    switch (sPhase) {
        case Phase::Verifying:
            verifySlice();
            break;

        case Phase::ApplyRequested: {
            // Re-verificare SHA-256 inainte de orice stergere flash.
            if (!acquireSd()) {
                fail("Card SD ocupat la pornirea instalarii");
                break;
            }
            uint8_t hash[32];
            const bool hashed = hashFirmwareFile(hash);
            releaseSd();
            if (!hashed || memcmp(hash, sManifest.sha256, sizeof(hash)) != 0) {
                fail("SHA-256 nu mai corespunde manifestului");
                break;
            }
            Serial.println("[OTA] hash confirmat; cer oprirea Core 1");
            sQuiescedAck.store(false, std::memory_order_relaxed);
            sQuiesceRequested.store(true, std::memory_order_release);
            sQuiesceDeadline = millis() + QUIESCE_TIMEOUT_MS;
            sPhase = Phase::Quiescing;
            break;
        }

        case Phase::Quiescing:
            if (sQuiescedAck.load(std::memory_order_acquire)) {
                // Core 1 a confirmat: SPI liber, fisier audio inchis.
                Serial.println("[OTA] Core 1 oprit; instalez...");
                watchdog_enable(2000, true);
                multicore_reset_core1();
                installFirmware();  // nu returneaza
            }
            if (static_cast<int32_t>(millis() - sQuiesceDeadline) > 0) {
                Serial.println("[OTA] Core 1 NU a confirmat oprirea");
                sQuiesceRequested.store(false, std::memory_order_release);
                fail("Core 1 nu a confirmat oprirea");
            }
            break;

        default:
            break;
    }
}

bool beginUpload(uint32_t totalBytes, const char* fileName) {
    if (sPhase != Phase::Idle && sPhase != Phase::Uploaded &&
        sPhase != Phase::Verified && sPhase != Phase::Failed &&
        sPhase != Phase::Completed) {
        setError("O alta operatiune de update este in curs");
        return false;
    }
    if (totalBytes == 0 || (totalBytes % 512) != 0) {
        setError("Dimensiune UF2 invalida");
        return false;
    }
    if (!SDCard::isReady()) {
        setError("Card SD indisponibil");
        return false;
    }
    // Oprim iesirea audio de pe Core 0 (proprietarul I2S) inainte de a
    // cere accesul exclusiv; fara sunet activ nu exista buzz la pauza DMA.
    Sound::stop();
    if (!acquireSd()) {
        setError("Card SD ocupat");
        return false;
    }

    SD.mkdir(UPDATE_DIR);
    SD.remove(FW_PATH);
    SD.remove(META_PATH);
    sManifestValid = false;
    sFile = SD.open(FW_PATH, FILE_WRITE);
    if (!sFile) {
        releaseSd();
        setError("Fisierul UF2 nu poate fi creat pe SD");
        return false;
    }

    snprintf(sFileName, sizeof(sFileName), "%s", fileName != nullptr ? fileName : "");
    for (size_t i = 0; sFileName[i] != '\0'; i++) {
        const char c = sFileName[i];
        const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                        (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
        if (!ok) sFileName[i] = '_';
    }
    parseCompatibility(sFileName);
    clearError();
    sTotalBytes = totalBytes;
    sProgressBytes = 0;
    sPhase = Phase::Uploading;
    Serial.printf("[OTA] upload start: %s (%lu octeti)\n",
                  sFileName, static_cast<unsigned long>(totalBytes));
    return true;
}

bool writeChunk(const uint8_t* data, size_t length) {
    if (sPhase != Phase::Uploading || data == nullptr || length == 0) return false;
    if (sProgressBytes + length > sTotalBytes) return false;
    if (sFile.write(data, length) != length) return false;
    sProgressBytes += length;
    return true;
}

void abortUpload() {
    if (sPhase != Phase::Uploading) return;
    sFile.close();
    SD.remove(FW_PATH);
    releaseSd();
    fail("Upload intrerupt");
}

bool endUpload() {
    if (sPhase != Phase::Uploading) return false;
    sFile.close();
    releaseSd();
    if (sProgressBytes != sTotalBytes) {
        SD.remove(FW_PATH);
        fail("Upload incomplet");
        return false;
    }
    sPhase = Phase::Uploaded;
    Serial.println("[OTA] upload complet");
    return true;
}

bool startVerify() {
    if (sPhase != Phase::Uploaded && sPhase != Phase::Verified) {
        setError("Nicio imagine incarcata de verificat");
        return false;
    }
    Sound::stop();
    if (!acquireSd()) {
        setError("Card SD ocupat");
        return false;
    }
    sFile = SD.open(FW_PATH, FILE_READ);
    if (!sFile) {
        releaseSd();
        setError("Fisierul UF2 nu se deschide");
        return false;
    }
    Serial.println("[OTA] verificare pornita");
    const uint32_t fileSize = sFile.size();
    if (fileSize == 0 || (fileSize % 512) != 0) {
        sFile.close();
        releaseSd();
        setError("Dimensiune UF2 invalida");
        return false;
    }

    memset(&sManifest, 0, sizeof(sManifest));
    sManifest.magic = MANIFEST_MAGIC;
    sManifest.version = MANIFEST_VERSION;
    snprintf(sManifest.fileName, sizeof(sManifest.fileName), "%s", sFileName);
    sManifest.fileSize = fileSize;
    sManifest.blockCount = fileSize / 512;
    sManifest.firstAddr = 0;
    sManifest.panelType = BuildInfo::PanelType;
    sManifest.hwVariant = BuildInfo::HardwareVariant;
    sManifest.fwMajor = sFwMajor;
    sManifest.fwMinor = sFwMinor;
    sManifest.compat = sCompatKnown ? (sCompat ? 1 : 2) : 0;

    // Prima adresa se citeste anticipat; validarea de secventa o cere.
    uint8_t first[512];
    if (sFile.read(first, sizeof(first)) != static_cast<int>(sizeof(first))) {
        sFile.close();
        releaseSd();
        setError("Citire UF2 esuata la verificare");
        return false;
    }
    const char* error = nullptr;
    if (readLe32(first) != UF2_MAGIC_START0 || (readLe32(first + 12) & 0xFF) != 0) {
        sFile.close();
        releaseSd();
        setError("Bloc UF2 cu magic invalid");
        return false;
    }
    sManifest.firstAddr = readLe32(first + 12);
    if (sManifest.firstAddr != APP_REGION_BEGIN) {
        sFile.close();
        releaseSd();
        setError("Imaginea nu incepe la baza flash-ului");
        return false;
    }
    if (readLe32(first + 24) != sManifest.blockCount) {
        sFile.close();
        releaseSd();
        setError("Numerotare blocuri incoerenta");
        return false;
    }
    sFile.seek(0);

    sSha.begin();
    sVerifyBlock = 0;
    sProgressBytes = 0;
    clearError();
    sPhase = Phase::Verifying;
    (void)error;
    return true;
}

bool requestApply() {
    if (sPhase != Phase::Verified || !sManifestValid) {
        setError("Imaginea nu este verificata");
        return false;
    }
    if (!ServiceMenu::isInServiceMode()) {
        setError("Instalarea cere Service Mode activ");
        return false;
    }
    if (sCompatKnown && !sCompat) {
        setError("Firmware pentru alt tip de panou");
        return false;
    }
    Sound::stop();
    clearError();
    sPhase = Phase::ApplyRequested;
    Serial.println("[OTA] apply cerut; re-verific SHA-256");
    return true;
}

void cancel() {
    if (sPhase == Phase::Quiescing || sPhase == Phase::Installing) return;
    if (sPhase == Phase::Uploading) {
        abortUpload();
        return;
    }
    if (sPhase == Phase::Verifying) {
        sFile.close();
        releaseSd();
    }
    clearError();
    sPhase = Phase::Idle;
}

const char* lastError() {
    return sError;
}

void getStatus(Status& out) {
    out.phase = sPhase;
    out.total = sTotalBytes;
    if (sPhase == Phase::Verifying) {
        out.total = sManifest.blockCount * 512;
    }
    out.bytes = sProgressBytes;
    snprintf(out.error, sizeof(out.error), "%s", sError);
    snprintf(out.fileName, sizeof(out.fileName), "%s", sFileName);
    out.compatibilityKnown = sCompatKnown;
    out.compatible = sCompat;
    out.bootConfirmed = sBootConfirmed;
    out.fwMajor = sFwMajor;
    out.fwMinor = sFwMinor;
}

bool pollQuiesce() {
    if (!sQuiesceRequested.load(std::memory_order_acquire)) return false;
    if (!sQuiescedAck.load(std::memory_order_relaxed)) {
        // Punct sigur: suntem intre chunk-uri SD; inchidem stream-ul audio
        // si eliberam SPI inainte de confirmare.
        SDCard::abortAudioStream();
        sQuiescedAck.store(true, std::memory_order_release);
    }
    // Asteapta resetarea din Core 0 (sau retragerea cererii la timeout).
    while (sQuiesceRequested.load(std::memory_order_acquire)) {
        tight_loop_contents();
    }
    sQuiescedAck.store(false, std::memory_order_relaxed);
    return true;
}

} // namespace UpdateService
