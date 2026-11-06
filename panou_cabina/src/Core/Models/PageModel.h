#ifndef PAGE_MODEL_H
#define PAGE_MODEL_H

#include <stdint.h>

enum class SemanticState : uint8_t {
    Neutral,
    Success,
    Warning,
    Alert
};

enum class PageLineType : uint8_t {
    Empty,
    Separator,
    DataText,
    DataNumeric,
    DataHex
};

enum class DiagnosticsProfile : uint8_t {
    Service,
    Developer
};

struct PageLine {
    PageLineType type;
    const char* labelKey;
    const char* valueText;
    uint32_t valueNum;
    SemanticState state;
};

constexpr uint8_t MAX_PAGE_LINES = 5;

struct PageModel {
    const char* title;
    uint8_t pageIndex;
    uint8_t totalPages;
    PageLine lines[MAX_PAGE_LINES];
};

#endif