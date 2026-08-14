#ifndef CHANGELOG_H
#define CHANGELOG_H

/* 

/* ============================================================================
   JURNAL DE MODIFICĂRI OFICIAL (CHANGELOG) - PLATFORMA SOFTWARE LIFT DUPLEX
   CONVENȚIE DE PRODUCȚIE: [+] Feature  |  [*] Refactor  |  [!] Fix
   ============================================================================

   ============================================================================
   VERSIUNEA CURENTĂ DE LUCRU: V10.25
   STATUS: DEVELOPMENT OPEN (Etapa: Activare Display real pe Core 0)
   ============================================================================
   Pending integration of semantic diagnostics into physical Adafruit displays...

   ============================================================================
   RELEASE VALIDAT ȘI ÎNGHEȚAT: V10.24
   STATUS: CERTIFIED BASELINE (Infrastructură de Meniu Service pe Paginare Logică)
   ============================================================================
   [+] FEATURE: Structură de Meniu Service 'DiagnosticsDisplay.h/.cpp'.
   [+] FEATURE: Implementat sistem de navigare ciclică 'nextPage()' / 'previousPage()'.
   [*] REFACTOR: Categorisire Logică Meniu DiagnosticsPage pe blocuri (System și Runtime).
   [*] REFACTOR: Adăugat constantele de capăt matematice 'FIRST_PAGE' și 'LAST_PAGE'.
   ============================================================================ */


============================================================================
   JURNAL DE MODIFICĂRI OFICIAL (CHANGELOG) - PLATFORMA SOFTWARE LIFT DUPLEX
   CONVENȚIE DE PRODUCȚIE: [+] Feature  |  [*] Refactor  |  [!] Fix
   ============================================================================

   ============================================================================
   VERSIUNEA CURENTĂ DE LUCRU: V10.24
   STATUS: DEVELOPMENT OPEN (Etapa: Implementare Interfețe Active DiagnosticsDisplay)
   ============================================================================
   Pending modular implementation of isolated graphic diagnostic components...

   ============================================================================
   RELEASE VALIDAT ȘI ÎNGHEȚAT: V10.23
   STATUS: CERTIFIED BASELINE (Arhitectură Semantică cu Șablon de Afișare Consolă)
   ============================================================================
   [+] FEATURE: Strat Structural Semantic 'ProcessedDiagnostics' (Pasul 1)
       - Modulul Diagnostics.cpp acționează ca un translator pur în memorie, lock-free.
       - Traduce datele brute (raw) direct în pointeri literali text stabili (const char*).
       - Eliminat enumerările intermediare double (...Text), optimizând utilizarea CPU și RAM.
       - Implementat modularizarea structurii prin compoziție ierarhizată pe obiecte (.lift1 și .lift2).
       
   [+] FEATURE: Consumator Dedicat de Consolă 'DiagnosticsConsole' (Pasul 2)
       - Transformat funcția 'render()' într-un șablon de afișare pur (Template View).
       - Introdus helperii statici: 'printResetReason()', 'printDirection()', 'printOccupancy()', 'printService()'.
       - Eliberat complet codul de switch-uri secundare parazite, transformându-l într-o structură auto-documentată.
       - Corectat critic dimensiunea bufferului local 'buf' (16 octeți) pentru a elimina riscul de HardFault în runtime.

   ============================================================================
   RELEASE VALIDAT ȘI ÎNGHEȚAT: V10.22
   STATUS: CERTIFIED BASELINE (Infrastructură Consolidată și Securizată)
   ============================================================================
   [+] FEATURE: LiftController pur tipizat și segmentat în 5 subrutine private:
       - 'updateButtonState()'  -> Gestiune filtru anti-stuck mecanic.
       - 'updateLeds()'         -> Mașină de stări optică și audio local palier.
       - 'selectLift()'         -> Algoritm duplex distanțe absolute.
       - 'updateWearBalancer()' -> Logica asincronă de rotație uzură uniformă.
       - 'processCall()'        -> Gestiune front butoane și ferestre de inhibiție.
   [!] FIX: Securizat parserul împotriva Buffer Overflow utilizând 'strncpy()' în loc de 'strcpy()'.
   [!] FIX: Implementat Sanity Check rigid în parser pe toate plajele de valori ale cabinelor (etaje 0-17).
   [*] REFACTOR: Externalizat timpii microsecundici de settle hardware ai MAX485 direct în Config::Protocol.

   ============================================================================
   RELEASE VALIDAT ȘI ÎNGHEȚAT: V10.21
   STATUS: CERTIFIED BASELINE (Tipurile Enum Class și Structura Config Ierarhică)
   ============================================================================
   [*] REFACTOR: Introducere Enumerări Puternic Tipizate (Direction, ServiceState, Occupancy).
   [*] REFACTOR: Namespace-ul Config ierarhizat structural pe sub-domenii (Hardware, Timing, Audio, Display, Protocol).
   [!] FIX: Aliniat istoricele Display.cpp la noile tipuri enum class pentru eliminarea flicker-ului.

   ============================================================================
   RELEASE VALIDAT ȘI ÎNGHEȚAT: V10.20
   STATUS: CERTIFIED BASELINE (Foundation Release)
   ============================================================================
   [*] REFACTOR: Arhitectură curată pe 3 niveluri (Drivers, Logic, Presentation).
   [*] REFACTOR: Eliminare directive comutabile provizorii (#ifdef Parter_si_17).
   [*] REFACTOR: Stabilizat pinii și magistralele native conform schemei V8.95 (SPI0/SPI1).
   [+] FEATURE: Sincronizare prin memorie partajată 'SharedMemory' și Seqlock pe 8 biți decuplat.
   [+] FEATURE: Contor de boot persistent în Flash ('bootCounter++' via EEPROM emulat).
   [+] FEATURE: Citire cauză hardware reset ('lastResetReason') direct din registrele native RP2040.
   ============================================================================ */

#endif // CHANGELOG_H
