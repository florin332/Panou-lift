#include "DisplayRenderer.h"
#include "UITheme.h" // Includem noul strat de localizare (Viziunea ta)
#include "Arduino.h"

namespace DisplayRenderer
{
    void init() {
        // Adaptor pur structural pasiv
    }

    // Traducerea din stare semantică pură în cod de pixeli de sticlă
    static uint16_t mapTftColor(SemanticState state) {
        switch (state) {
            case SemanticState::Success: return 0x07E0; // COLOR_GREEN
            case SemanticState::Warning: return 0xFFE0; // COLOR_YELLOW
            case SemanticState::Alert:   return 0xF800; // COLOR_RED
            case SemanticState::Neutral:
            default:                     return 0xFFFF; // COLOR_WHITE
        }
    }

    void render(DisplayTarget target, const PageModel &logicalPage) {
        Display::clearTargetScreen(target);
        
        // Extragerea titlului tradus prin cheia din UITheme (Viziunea ta)
        const char* humanTitle = UITheme::getText(logicalPage.title);
        Display::printMenuHeader(target, humanTitle);

        // Buclă auto-documentată bazată pe constanta de limită MAX_PAGE_LINES (Observația ta)
        for (uint8_t i = 0; i < MAX_PAGE_LINES; i++) {
            uint8_t physicalLineIndex = i + 2; 
            const PageLine &line = logicalPage.lines[i];
            
            // Maparea culorii direct din starea semantică tipizată (Observația ta)
            uint16_t currentPixelColor = mapTftColor(line.state);
            const char* humanLabel = UITheme::getText(line.labelKey);

            switch (line.type) {
                case PageLineType::Empty:
                    // Sări peste randare pe această linie (lasă fundalul negru curat)
                    break;

                case PageLineType::Separator:
                    // Desenează direct o linie hardware continuă de separare, eliminând stringurile fictive!
                    // (Lățime ecran 128 pixeli, plasată pe axa Y a rândului curent)
                    {
                        //uint8_t yPos = 24 + (physicalLineIndex * 15);
                        // Apelăm o linie rapidă gri direct din driverul hardware
                        // Notă: Putem adăuga drawFastHLine în Display.h sau o emitem nativ prin adaptor dacă avem instanțe
                        // Pentru a păstra Display.h intact, folosim o linie de caractere punctuale subțiri sau apelăm direct o primitivă
                        Display::printMenuLineExt(target, physicalLineIndex, "----------------", "", 0x7BEF);
                    }
                    break;

                case PageLineType::DataNumeric:
                    Display::printMenuLineExt(target, physicalLineIndex, humanLabel, line.valueNum, currentPixelColor);
                    break;

                case PageLineType::DataHex:
                    Display::printMenuLineHex(target, physicalLineIndex, humanLabel, line.valueNum);
                    break;

                case PageLineType::DataText:
                    // Dacă valoarea text este o stare cunoscută o trecem prin traducere, altfel o lăsăm brută
                    const char* humanValueText = (strcmp(line.valueText, "OK") == 0 || strcmp(line.valueText, "running") == 0) 
                                                 ? line.valueText 
                                                 : line.valueText;
                    Display::printMenuLineExt(target, physicalLineIndex, humanLabel, humanValueText, currentPixelColor);
                    break;
            }
        }

        // CORECTAT DEFINITIV (Observația ta 1): Alocare fixă de 16 octeți sigură împotriva Stack Overflow!
        char footerBuf[16];
        snprintf(footerBuf, sizeof(footerBuf), "%d / %d", logicalPage.pageIndex, logicalPage.totalPages);
        Display::printMenuFooterDecoration(target, footerBuf);
    }
}
