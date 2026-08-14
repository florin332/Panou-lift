#ifndef DISPLAY_RENDERER_H
#define DISPLAY_RENDERER_H

#include "DiagnosticsPages.h" // Pentru a recunoaște structura PageModel
#include "Drivers/Display.h" // Pentru a recunoaște DisplayTarget

namespace DisplayRenderer
{
    void init();

    // ADAPTORUL PASIV PUR (Viziunea ta)
    // Preia modelul logic abstract PageModel și îl traduce mecanic în primitive Display.h
    void render(DisplayTarget target, const PageModel &logicalPage);
}

#endif // DISPLAY_RENDERER_H
