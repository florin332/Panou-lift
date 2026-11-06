#ifndef DISPLAY_RENDERER_H
#define DISPLAY_RENDERER_H

#include "PageModel.h"
#include "Graphics.h"

namespace DisplayRenderer
{
    void init();

    void showOverloadImage();
    void hideOverloadImage();
    void showBacklight();

    // ADAPTORUL PASIV PUR (Viziunea ta)
    // Preia modelul logic abstract PageModel și îl traduce mecanic în primitive Display.h
    void render(GraphicsTarget target, const PageModel &logicalPage);
}

#endif // DISPLAY_RENDERER_H
