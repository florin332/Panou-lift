// CLASA LOGIC: Logic/Application.h

#ifndef APPLICATION_H
#define APPLICATION_H

namespace Application
{
    // Inițializează serviciile și publică starea inițială a aplicației.
    void init();

    // Inițializează hardware-ul de prezentare pe Core 1.
    void initCore1();

    // Core 0: evenimente, audio, transport și actualizarea modelului.
    void runCore0();

    // Core 1: LCD și prezentare.
    void runCore1();
}

#endif // APPLICATION_H
