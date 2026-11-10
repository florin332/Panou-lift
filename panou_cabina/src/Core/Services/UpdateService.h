#ifndef UPDATE_SERVICE_H
#define UPDATE_SERVICE_H

#include <cstddef>
#include <cstdint>

// Update firmware prin SD: upload UF2 (web) -> verificare locala ->
// instalare locala din SD. Flux separat in faze; instalarea ruleaza
// exclusiv pe Core 0 dupa oprirea gratuitoasa a Core 1.
namespace UpdateService {

enum class Phase : uint8_t {
    Idle = 0,
    Uploading,
    Uploaded,
    Verifying,
    Verified,
    ApplyRequested,
    Quiescing,
    Installing,
    Failed,
    Completed
};

struct Status {
    Phase phase = Phase::Idle;
    uint32_t bytes = 0;        // progres in faza curenta
    uint32_t total = 0;        // totalul fazei curente
    char error[48] = {};
    char fileName[33] = {};
    bool compatibilityKnown = false;
    bool compatible = false;
    bool bootConfirmed = false;
    uint8_t fwMajor = 0;
    uint8_t fwMinor = 0;
};

void init();                 // Core 0, la pornire
void update();               // Core 0 loop

// Apelate din WebUi (Core 0)
bool beginUpload(uint32_t totalBytes, const char* fileName);
bool writeChunk(const uint8_t* data, size_t length);
void abortUpload();
bool endUpload();
bool startVerify();          // Uploaded/Verified -> Verifying (feliat)
bool requestApply();         // Verified -> ApplyRequested (hash + quiesce)
void cancel();               // -> Idle (fisierele raman pe SD)
const char* lastError();

void getStatus(Status& out);

// Hook pentru Core 1: returneaza true daca nucleul a fost parcat.
// La cerere: inchide stream-ul audio, confirma, apoi asteapta resetarea.
bool pollQuiesce();

} // namespace UpdateService

#endif // UPDATE_SERVICE_H
