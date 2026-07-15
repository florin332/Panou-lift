#ifndef DIAGNOSTICS_PAGES_H
#define DIAGNOSTICS_PAGES_H

#include "Diagnostics.h"

// 1. Stări semantice pure încapsulate (Înlocuiesc strcmp-urile cromatico-textuale)
enum class SemanticState : uint8_t {
    Neutral,   // Text informativ curent, etichete standard (Alb)
    Success,   // Regim normal, conectat, liber (Verde)
    Warning,   // Revizie, ocupat, temporizare (Galben)
    Alert      // Defect, eroare CRC, lipsă serial (Roșu)
};

// 2. Tipuri de linii logice pentru Design System Engine (Elimină elementele text-fantomă)
enum class PageLineType : uint8_t {
    Empty,        // Spațiu liber de respirație vizuală
    Separator,    // Linie hardware orizontală continuă
    DataText,     // Conține Cheie/Label + Valoare String
    DataNumeric,  // Conține Cheie/Label + Valoare Întreagă
    DataHex       // Conține Cheie/Label + Valoare Hexazecimală 0x
};

enum class DiagnosticsProfile : uint8_t {
    Service,
    Developer
};

// 3. Structura unui rând logic pur de date
struct PageLine {
    PageLineType type;
    const char* labelKey; // Acționează ca ID simbolic pentru dicționar/UITheme
    const char* valueText;
    uint32_t valueNum;
    SemanticState state;
};

// 4. Eliminarea numărului magic global (Centralizat pe proiect)
constexpr uint8_t MAX_PAGE_LINES = 5;

struct PageModel {
    const char* title;
    uint8_t pageIndex;
    uint8_t totalPages;
    PageLine lines[MAX_PAGE_LINES]; // Alocare fixă deterministă bazată pe constantă
};

namespace DiagnosticsPages
{
    enum class ServicePage : uint8_t { System, Lift1, Lift2, Communication, Version };
    enum class DevPage : uint8_t { Engineering, MemoryLayout, Tasks, ProtocolStats, Assertions };

    constexpr uint8_t TOTAL_SERVICE_PAGES = 5;
    constexpr uint8_t TOTAL_DEV_PAGES     = 5;

    void init();
    void buildServicePage(const ProcessedDiagnostics &info, ServicePage page, PageModel &output);
    void buildDeveloperPage(const ProcessedDiagnostics &info, DevPage page, PageModel &output);
}

#endif // DIAGNOSTICS_PAGES_H
