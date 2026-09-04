# Cerințe UI – Service Box

## 1. Scop

Definirea interfeței grafice și a fluxului de navigare pentru Service Box.

Specificația UI urmărește structura definită în:

`service_box/svcbox_menu.drawio`

Diagrama `svcbox_menu.drawio` este referința pentru:

- structura meniului;
- paginile UI;
- acțiunile utilizatorului;
- tranzițiile dintre pagini;
- operațiile de service reprezentate în UI.

În această etapă se implementează mai întâi interfața grafică și navigarea.

Comenzile seriale către panouri și răspunsurile panourilor vor fi integrate ulterior printr-un nivel separat de service/protocol.

Nu se adaugă funcționalități, pagini, butoane sau informații care nu sunt definite în specificație sau în `svcbox_menu.drawio`.

## 2. Display

- Rezoluție logică țintă: 240x320 pixeli.
- Orientare: portret.
- Toate platformele hardware utilizează aceeași interfață logică.
- Codul specific display-ului aparține HAL.
- Codul specific touchscreen-ului aparține HAL.
- Codul UI nu trebuie să conțină tratare specifică hardware pentru display sau touchscreen.

Platforme hardware:

1. Waveshare RP2350-Touch-LCD-2.8
2. GroundStudio Marble Pico + TFT 3.2"

Ambele platforme trebuie să ofere aceeași experiență de utilizare.

Logica meniului nu trebuie duplicată pentru diferite platforme hardware.

# 3. Structura generală a meniului

Fluxul principal este:

POWER ON
   ↓
BOOT SEQUENCE
   ↓
BATTERY CHECK
   ↓
START
   ↓
HANDSHAKE / PANEL IDENTIFICATION
   ↓
MODE SELECTION

`BOOT SEQUENCE` este o secvență internă de inițializare și nu este o pagină reală.

`BATTERY CHECK` este o pagină reală.

`START` este o pagină reală.

`MODE SELECTION` este meniul principal de service.

# 4. Boot Sequence

Boot Sequence reprezintă inițializarea sistemului.

Nu este o pagină navigabilă.

Nu trebuie să introducă interacțiune cu utilizatorul.

După finalizarea inițializării se trece la:

Battery Check

# 5. Battery Check

Pagina `Battery Check` este afișată după pornirea sistemului și înainte de pagina `Start`.

Scop:

- citirea nivelului bateriei;
- afișarea nivelului bateriei;
- detectarea stării de încărcare.

## 5.1 Stare normală

Când Service Box nu este în încărcare, pagina afișează:

BATTERY LEVEL

     XX %

Procentul trebuie să fie afișat clar și vizibil în zona centrală a ecranului.

## 5.2 Indicarea nivelului bateriei

Nivelul bateriei este indicat și prin culoarea valorii:

- nivel scăzut → ROȘU;
- nivel mediu → GALBEN;
- nivel bun → VERDE.

Pragurile numerice exacte pentru cele trei niveluri vor fi stabilite separat.

Conversia tensiunii bateriei în procent nu aparține UI-ului.

UI-ul primește de la nivelul hardware/application:

- procentul bateriei;
- starea de încărcare.

## 5.3 Charging

Dacă Service Box este în încărcare, pagina afișează:

CHARGING

În această stare:

- nu se afișează procentul bateriei;
- nu se face trecerea automată la Start;
- nu se aplică temporizarea normală de 5 secunde;
- pagina rămâne afișată cât timp `CHARGING` este activ.

Când încărcarea se termină, pagina revine la comportamentul normal.

## 5.4 Temporizare

Dacă dispozitivul NU este în stare `CHARGING`:

1. se citește starea bateriei;
2. se afișează procentul;
3. pagina rămâne afișată 5 secunde;
4. după 5 secunde se trece automat la `Start`.

## 5.5 Interacțiune

Battery Check este o pagină informativă.

Nu conține:

- butoane;
- comenzi;
- meniuri;
- gesturi;
- controale touchscreen.

# 6. Start

Pagina `Start` este prima pagină principală a Service Box.

Ea afișează informațiile disponibile despre:

- Service Box;
- firmware;
- card SD;
- panoul conectat;
- etajul curent.

Pagina trebuie să conțină butonul:

START

## 6.1 Handshake

Acțiunea `START` lansează handshake-ul cu panoul conectat.

Handshake-ul trebuie să permită identificarea:

- tipului de panou;
- valorii curente a etajului.

Dacă nu există răspuns de la panou:

unknown panel

Dacă etajul nu este configurat:

floor: ?

Pagina trebuie să poată reprezenta cel puțin:

panel type: unknown
floor: ?

sau informațiile identificate corespunzător.

Detaliile comenzilor seriale folosite pentru handshake nu aparțin rendererului UI.

# 7. Mode Selection

După acțiunea `START` și procesarea handshake-ului, Service Box intră în:

MODE SELECTION

Aceasta este pagina principală de service.

Meniul trebuie să permită accesul la funcțiile definite în diagrama `svcbox_menu.drawio`.

Funcțiile definite sunt:

- Exit Service Mode;
- Flash;
- Floor Set;
- Communication;
- Display;
- MCU INFO.

Nu se adaugă alte funcții până când acestea nu sunt definite în `svcbox_menu.drawio`.

# 8. Exit Service Mode

Acțiunea `Exit Service Mode` revine la pagina `Start`.

Flux:

MODE SELECTION
      ↓
EXIT SERVICE MODE
      ↓
START

Nu este necesară o pagină intermediară.