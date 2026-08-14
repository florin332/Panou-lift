#ifndef UI_PRESENTER_H
#define UI_PRESENTER_H

#include "DiagnosticsPages.h" // Recunoaște DiagnosticsPage și profilurile existente

namespace UIPresenter
{
    // Inițializează stratul de prezentare abstractă
    void init();

    // CONSTRUCTORUL PUR DE PAGINI LOGICE (Viziunea ta)
    // Consumă datele interpretate și populează un obiect imutabil PageModel în memorie.
    // Nu știe ce este un pixel, un ecran TFT sau o conexiune SPI.
    void buildPage(
        const ProcessedDiagnostics &info,
        DiagnosticsProfile profile,
        uint8_t pageIndex,
        PageModel &outputModel
    );
}

#endif // UI_PRESENTER_H
