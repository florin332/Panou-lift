// CLASA LOGIC: Logic/Application.h

#ifndef APPLICATION_H
#define APPLICATION_H

namespace Application
{
    // Inițializează serviciile și publică starea inițială a aplicației.
    void init();

    // Deleagă ciclul Core 0 către controller-ul de prezentare.
    void runCore0();

    // Orchestrează serviciile de lift și alarmă și publică starea actualizată.
    void runCore1();
}

#endif // APPLICATION_H
