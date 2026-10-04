// CLASA LOGIC: Logic/Application.h

#ifndef APPLICATION_H
#define APPLICATION_H

namespace Application
{
    // Inițializează memoria partajată globală și orchestrează pornirea tuturor modulelor din subordine
    void init();

    // Sarcina ciclică dedicată exclusiv pentru Core 0 (Afișaj, Screensaver, Sound, LEDs)
    void runCore0();

    // Sarcina ciclică dedicată exclusiv pentru Core 1 (Protocol, Calcul comenzi, Actualizare RAM)
    void runCore1();
}

#endif // APPLICATION_H
