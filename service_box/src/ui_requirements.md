# Cerințe UI – Service Box

## 1. Scop

Definirea interfeței grafice și a fluxului de navigare pentru Service Box.

Structura meniului și fluxurile de navigare sunt definite în:

`service_box/svcbox_menu.drawio`

`svcbox_menu.drawio` este referința principală pentru:

- paginile UI;
- ordinea și structura meniului;
- acțiunile disponibile;
- tranzițiile dintre pagini;
- secvențele interne asociate operațiilor de service.

`ui_requirements.md` definește cerințele de implementare ale interfeței grafice pe baza acestei diagrame.

UI-ul trebuie să fie independent de hardware-ul concret al Service Box.

Suportul pentru diferitele platforme hardware se realizează prin HAL.

Operațiile de service și comunicația cu panoul sunt separate de rendererul UI și vor fi integrate progresiv.

Nu se introduc pagini, funcții, butoane sau fluxuri care nu sunt definite în `svcbox_menu.drawio` sau aprobate explicit ulterior.

## 2. Display și hardware UI

Service Box trebuie să ofere aceeași interfață logică pe toate platformele hardware suportate.

Platformele sunt:

1. Waveshare RP2350 Touch LCD 2.8"
2. GroundStudio Marble Pico + TFT 3.2"

Rezoluția și orientarea efectivă a display-ului sunt gestionate de HAL și de configurația specifică platformei.

UI-ul nu trebuie să depindă direct de:

- driverul LCD;
- controllerul LCD;
- magistrala SPI;
- controllerul touchscreen;
- magistrala I2C;
- GPIO-uri;
- implementarea hardware specifică platformei.

Codul specific display-ului și touchscreen-ului aparține HAL.

Pentru Waveshare, touchscreen-ul CST328 este hardware validat și este conectat prin I2C conform `hardware_map.md`.

UI-ul trebuie să primească de la HAL:

- dimensiunea logică a suprafeței de afișare;
- coordonatele tactile;
- starea touchscreen-ului;
- evenimentele de touch.

Logica meniului trebuie să fie comună pentru platformele hardware suportate.

Nu se creează versiuni separate ale meniului pentru Waveshare și Marble.

## 3. Structura generală a meniului

Fluxul principal al Service Box este:

POWER ON
   ↓
BOOT SEQUENCE
   ↓
BATTERY CHECK
   ↓
CAL PIN CHECK
   ↓
START
   ↓
HANDSHAKE / PANEL IDENTIFICATION
   ↓
MODE SELECTION

`BOOT SEQUENCE` este o secvență internă de inițializare și nu este o pagină UI.

`BATTERY CHECK` este o pagină UI.

`CAL PIN CHECK` este o verificare de stare hardware și nu este o pagină UI.

Dacă `CAL PIN` indică modul de calibrare, se execută:

CAL PIN = LOW
   ↓
TOUCH CALIBRATION
   ↓
START

Dacă `CAL PIN` indică funcționarea normală:

CAL PIN = HIGH
   ↓
5 secunde
   ↓
START

După `START`, Service Box efectuează handshake-ul cu panoul conectat.

În funcție de rezultatul handshake-ului:

- fără răspuns de la panou → `Panel Type Selection`;
- tip de panou identificat și etaj neconfigurat → `Current Floor Set`;
- tip de panou identificat și etaj configurat → `Mode Selection`.

Structura exactă a tranzițiilor trebuie să respecte `service_box/svcbox_menu.drawio`.

## 4. Boot Sequence

`Boot Sequence` este secvența de inițializare executată la pornirea Service Box.

Nu este o pagină UI și nu este accesibilă prin navigarea touchscreen.

Secvența trebuie să inițializeze componentele necesare funcționării Service Box.

După finalizarea inițializării se trece la:

`Battery Check`

Boot Sequence nu trebuie să solicite intervenția utilizatorului.

## 5. Battery Check

Pagina `Battery Check` este afișată după `Boot Sequence`.

Scopul paginii este:

- afișarea nivelului bateriei;
- indicarea stării de încărcare.

## 5.1 Funcționare normală

Dacă Service Box nu este conectat la încărcător, pagina afișează:

BATTERY LEVEL

     XX %

Valoarea bateriei trebuie afișată clar și vizibil.

Nivelul bateriei este indicat prin culoare:

- nivel scăzut → roșu;
- nivel mediu → galben;
- nivel bun → verde.

Pragurile exacte pentru aceste niveluri vor fi definite separat.

Calculul procentului bateriei nu aparține UI-ului.

UI-ul primește valoarea calculată de nivelul hardware/application.

## 5.2 Charging

Dacă Service Box este în stare de încărcare, pagina afișează:

CHARGING

În această stare:

- nu se face trecerea automată la `Start`;
- pagina rămâne afișată cât timp starea de încărcare este activă.

După încetarea încărcării, se revine la funcționarea normală a paginii.

## 5.3 Temporizare

Dacă Service Box nu este în stare `CHARGING`:

1. se citește starea bateriei;
2. se afișează nivelul bateriei;
3. pagina rămâne afișată 5 secunde;
4. după 5 secunde se trece la `Start`.

## 5.4 Touchscreen

`Battery Check` nu necesită interacțiune touchscreen.

Nu conține butoane sau controale tactile.

## 5.5 CAL PIN

După perioada de afișare a `Battery Check`, se verifică starea `CAL PIN`.

Dacă:

`CAL PIN = HIGH`

se continuă fluxul normal către `Start`.

Dacă:

`CAL PIN = LOW`

se intră în procedura:

`Touch Calibration`

După finalizarea procedurii de calibrare se trece la `Start`.

## 6. Touch Calibration

`Touch Calibration` este o procedură specială de inițializare a Service Box.

Procedura este declanșată prin `CAL PIN` și nu este accesibilă din meniul normal de service.

## 6.1 Activarea calibrării

După `Battery Check`:

- dacă `CAL PIN = HIGH` → se continuă către `Start`;
- dacă `CAL PIN = LOW` → se execută `Touch Calibration`.

Flux:

CAL PIN = LOW
   ↓
TOUCH CALIBRATION
   ↓
START

## 6.2 Procedura de calibrare

Procedura de calibrare trebuie să permită determinarea parametrilor necesari pentru transformarea coordonatelor brute ale touchscreen-ului în coordonate UI.

Procedura trebuie să:

- afișeze ținte de calibrare;
- solicite atingerea succesivă a țintelor;
- colecteze coordonatele furnizate de touchscreen;
- determine parametrii de calibrare;
- salveze rezultatul calibrării;
- utilizeze calibrarea rezultată pentru navigarea UI.

Algoritmul exact și numărul de puncte de calibrare vor fi definite separat.

## 6.3 Hardware

Calibrarea trebuie să fie independentă de controllerul touchscreen utilizat.

Implementarea hardware-specifică aparține HAL.

UI-ul trebuie să lucreze cu coordonate tactile normalizate/logice.

Pentru Waveshare, touchscreen-ul CST328 este hardware validat conform `hardware_map.md`.

## 6.4 Finalizare

După finalizarea cu succes a calibrării:

`Touch Calibration → Start`

Dacă procedura eșuează, UI-ul trebuie să permită tratarea unei stări de eroare fără blocarea permanentă a Service Box.

Detaliile comportamentului la eroare vor fi definite odată cu implementarea procedurii de calibrare.

## 7. Start

Pagina `Start` este afișată după `Battery Check` și după efectuarea handshake-ului cu panoul conectat.

Handshake-ul trebuie efectuat înainte ca pagina `Start` să fie afișată, astfel încât informațiile despre panou să fie deja cunoscute atunci când utilizatorul ajunge pe această pagină.

Pagina `Start` afișează informațiile disponibile despre Service Box și panoul conectat, inclusiv:

- tipul panoului;
- etajul curent;
- informațiile disponibile despre firmware-ul Service Box;
- starea cardului SD;
- starea conexiunii cu panoul.

Dacă nu este primit niciun răspuns de la panou, pagina afișează:

panel type: unknown
floor: ?

Dacă panoul este identificat, se afișează tipul identificat.

Dacă panoul este identificat, dar etajul nu este configurat, se afișează:

floor: ?

Dacă etajul este configurat, se afișează valoarea curentă.

Pagina conține butonul `START`.

Apăsarea butonului `START` nu inițiază handshake-ul. Butonul utilizează rezultatul handshake-ului deja obținut pentru determinarea următoarei pagini.

Dacă nu există răspuns de la panou, apăsarea `START` deschide `Panel Type Selection`.

Dacă panoul este identificat, dar etajul nu este configurat, apăsarea `START` deschide `Current Floor Set`.

Dacă panoul este identificat și etajul este configurat, apăsarea `START` deschide `Mode Selection`.

Fluxul este:

BATTERY CHECK
↓
CAL PIN CHECK
↓
HANDSHAKE / PANEL IDENTIFICATION
↓
START
↓
START BUTTON
↓
în funcție de rezultatul handshake-ului:
- no handshake → PANEL TYPE SELECTION
- panel identified + floor not set → CURRENT FLOOR SET
- panel identified + floor set → MODE SELECTION

Detaliile comenzilor și răspunsurilor utilizate pentru handshake aparțin nivelului Service/Protocol și nu trebuie implementate direct în UI.

## 8. Exit Service Mode

Acțiunea `Exit Service Mode` este disponibilă din `Mode Selection`.

La selectarea acestei acțiuni, Service Box părăsește meniul de service și revine la pagina `Start`.

Fluxul este:

MODE SELECTION
↓
EXIT SERVICE MODE
↓
START

Nu este necesară o pagină intermediară.

După revenirea din `Floor Set` sau după finalizarea operației `Flash`, înainte de afișarea din nou a paginii `Start`, Service Box trebuie să reia handshake-ul cu panoul.

Reluarea handshake-ului este obligatorie pentru actualizarea stării panoului și a identificatorilor afișați pe pagina `Start`.

Fără reluarea handshake-ului, Service Box poate păstra informații vechi și poate reintra într-un flux incorect sau într-o buclă de navigare.

Prin urmare:

- `Exit Service Mode` → `Start`, fără handshake suplimentar;
- `Floor Set` → handshake → `Start`;
- `Flash` → handshake → `Start`.

Fluxurile sunt:

MODE SELECTION
↓
EXIT SERVICE MODE
↓
START

MODE SELECTION
↓
FLOOR SET
↓
CURRENT FLOOR SET
↓
HANDSHAKE
↓
START

MODE SELECTION
↓
FLASH
↓
PANEL TYPE SELECTION
↓
SELECT FIRMWARE
↓
CONFIRM & FLASH
↓
TARGET FLASH SEQUENCE
↓
HANDSHAKE
↓
START 

UI-ul trebuie doar să efectueze tranziția către pagina `Start`; gestionarea comunicației și a stării panoului aparține nivelului Service/Protocol.

## 9. Flash

Acțiunea `Flash` poate fi accesată în două situații:

- din `Mode Selection`, ca funcție normală de service;
- direct din `Start`, atunci când handshake-ul nu identifică panoul și este afișat `unknown panel`.

În ambele cazuri, selectarea `Flash` deschide pagina `Panel Type Selection`.

Fluxul din `Mode Selection` este:

MODE SELECTION
↓
FLASH
↓
PANEL TYPE SELECTION

Fluxul pentru un panou neidentificat este:

START
↓
UNKNOWN PANEL
↓
FLASH
↓
PANEL TYPE SELECTION

Procesul complet de flash este:

PANEL TYPE SELECTION
↓
SELECT FIRMWARE
↓
CONFIRM & FLASH
↓
TARGET FLASH SEQUENCE
↓
HANDSHAKE
↓
START

După finalizarea operației de flash, Service Box trebuie să reia handshake-ul cu panoul înainte de revenirea la `Start`.

Reluarea handshake-ului este obligatorie deoarece operația de flash poate modifica firmware-ul și starea panoului.

UI-ul nu trebuie să implementeze direct operația de flash sau protocolul utilizat pentru aceasta. UI-ul solicită operația prin nivelul Service și afișează starea și rezultatul furnizate de acesta.

## 10. Panel Type Selection

Pagina `Panel Type Selection` permite selectarea manuală a tipului de panou pentru operația care urmează.

Pagina este accesată:

- din `Flash`, după selectarea funcției;
- din `Start`, dacă handshake-ul nu a identificat panoul și utilizatorul alege să continue cu `Flash`.

Tipurile de panou disponibile sunt:

- `DUPLEX`;
- `SIMPLEX`;
- `CAB`.

Selecția tipului de panou determină firmware-urile disponibile în pagina `Select Firmware`.

Tipul selectat trebuie păstrat pe durata întregului flux de flash și trebuie transmis nivelului Service.

Pagina nu efectuează direct comunicație cu panoul și nu execută operația de flash.

După selectarea tipului de panou se trece la `Select Firmware`.

## 11. Select Firmware

Pagina `Select Firmware` afișează firmware-urile disponibile pe cardul SD pentru tipul de panou selectat anterior.

Sunt afișate numai fișierele firmware corespunzătoare tipului de panou selectat.

Fișierele firmware utilizate pentru operația de flash sunt fișiere:

`*.uf2`

Utilizatorul selectează firmware-ul dorit din lista disponibilă.

Dacă nu există niciun firmware corespunzător tipului de panou selectat, pagina trebuie să afișeze o stare corespunzătoare și să nu permită continuarea către operația de flash.

Numele fișierului selectat trebuie păstrat pentru pagina `Confirm & Flash`.

UI-ul nu trebuie să inventeze firmware-uri și nu trebuie să modifice conținutul fișierelor `.uf2`.

Accesul la cardul SD și enumerarea fișierelor aparțin nivelului Service/Application, nu rendererului UI.

După selectarea unui firmware valid se trece la `Confirm & Flash`.

## 12. Confirm & Flash

Pagina `Confirm & Flash` este etapa de confirmare înainte de executarea operației de flash.

### 12.1 Scop

Pagina trebuie să prezinte firmware-ul selectat și să permită lansarea operației de flash numai pentru ținta corespunzătoare.

### 12.2 Elemente afișate

Pagina trebuie să afișeze:

- numele firmware-ului selectat;
- informația privind ținta/panoul pentru care este destinat firmware-ul;
- avertizare dacă firmware-ul selectat nu corespunde țintei.

### 12.3 Buton Flash

Pagina conține butonul `FLASH`.

La apăsarea butonului:

1. UI verifică faptul că există un firmware valid selectat;
2. UI verifică faptul că firmware-ul este destinat țintei corespunzătoare;
3. dacă ținta este greșită, operația nu se execută și se afișează avertizarea corespunzătoare;
4. dacă verificările sunt valide, se lansează secvența de flash.

UI nu execută direct operația de programare și nu implementează protocolul de flash. Execuția efectivă este responsabilitatea layerului Service/Application.

### 12.4 Indicatori în timpul operației

În timpul secvenței de flash, UI trebuie să indice clar că operația este în desfășurare.

Indicatorul este animat:

`FLASHING .`
`FLASHING ..`
`FLASHING ...`

Animația continuă până când layerul Service confirmă finalizarea efectivă a operației de flash.

În această stare:

- butonul `FLASH` este dezactivat;
- navigarea către alte pagini este dezactivată;
- utilizatorul trebuie să poată vedea clar că operația este încă în desfășurare.

Indicatorul nu reprezintă un procentaj de progres decât dacă layerul Service furnizează explicit o valoare reală de progres.

### 12.5 Reconnect după flash

Imediat după finalizarea operației de flash, indicatorul se schimbă din `FLASHING` în `RECONNECT`.

Indicatorul este animat:

`RECONNECT .`
`RECONNECT ..`
`RECONNECT ...`

În această stare, Service Box:

1. așteaptă repornirea/reinițializarea panoului;
2. execută un nou handshake;
3. identifică din nou panoul;
4. actualizează informațiile despre panou, firmware și floor;
5. afișează pagina `Start` numai după finalizarea cu succes a reconectării și a handshake-ului.

Service Box nu trebuie să reutilizeze informațiile memorate înainte de flash.

Fluxul complet este:

`Confirm & Flash → FLASHING → RECONNECT → Handshake / Panel Identification → Start`

## 13. Target Flash Sequence

Secvența de flash este o operație internă și nu reprezintă o pagină UI separată.

În timpul execuției, UI trebuie să afișeze starea `FLASHING` definită la secțiunea 12.

După finalizarea efectivă a operației de flash:

1. indicatorul `FLASHING` se schimbă în `RECONNECT`;
2. Service Box așteaptă repornirea/reinițializarea panoului;
3. Service Box execută obligatoriu un nou handshake;
4. informațiile despre panou, firmware și floor sunt actualizate;
5. numai după finalizarea cu succes a reconectării și handshake-ului se afișează pagina `Start`.

În timpul stării `RECONNECT` se utilizează un indicator animat:

`RECONNECT .`
`RECONNECT ..`
`RECONNECT ...`

Navigarea către alte pagini și inițierea altor operații trebuie să rămână dezactivate până la finalizarea reconectării.

Fluxul complet este:

`Confirm & Flash → FLASHING → RECONNECT → Handshake / Panel Identification → Start`

Dacă reconectarea sau handshake-ul nu reușește, Service Box nu trebuie să afișeze `Start` folosind informațiile vechi. Eroarea și comportamentul ulterior vor fi definite de logica de comunicație/service.

## 14. Current Floor Set

Pagina `Current Floor Set` permite setarea numărului etajului pentru panoul identificat.

### 14.1 Acces

Pagina poate fi accesată în două situații:

- direct din `Start`, dacă panoul a fost identificat, dar floor nu este setat;
- din `Mode Selection`, prin funcția de setare a floor-ului.

Dacă panoul nu a fost identificat, funcția `Current Floor Set` nu este disponibilă.

### 14.2 Setarea floor-ului

Pagina trebuie să permită selectarea și setarea valorii corespunzătoare floor-ului.

Operația efectivă de scriere a valorii în panou este responsabilitatea layerului Service/Application. UI-ul gestionează afișarea și interacțiunea cu utilizatorul.

### 14.3 Indicator în timpul setării

În timpul operației de setare a floor-ului, UI trebuie să indice clar că operația este în desfășurare.

Se utilizează un indicator animat:

`SETTING FLOOR .`
`SETTING FLOOR ..`
`SETTING FLOOR ...`

În această stare:

- comanda de setare nu poate fi lansată repetat;
- navigarea către alte pagini este dezactivată;
- animația continuă până când layerul Service confirmă finalizarea operației.

Indicatorul nu reprezintă un procentaj de progres.

### 14.4 Reconnect după setarea floor-ului

După finalizarea operației de setare a floor-ului, indicatorul se schimbă în `RECONNECT`.

Se utilizează un indicator animat:

`RECONNECT .`
`RECONNECT ..`
`RECONNECT ...`

În această stare, Service Box:

1. așteaptă repornirea/reinițializarea panoului, dacă este necesară;
2. execută obligatoriu un nou handshake;
3. identifică din nou panoul;
4. actualizează informațiile despre panou și floor;
5. afișează pagina `Start` numai după finalizarea cu succes a reconectării și handshake-ului.

Service Box nu trebuie să reutilizeze informațiile memorate înainte de setarea floor-ului.

Fluxul complet este:

`Current Floor Set → SETTING FLOOR → RECONNECT → Handshake / Panel Identification → Start`

Dacă reconectarea sau handshake-ul nu reușește, Service Box nu trebuie să afișeze `Start` folosind informațiile vechi. Eroarea și comportamentul ulterior vor fi definite de logica de comunicație/service.

## 15. Mode Selection

Pagina `Mode Selection` este meniul principal pentru operațiile de service disponibile după identificarea panoului și confirmarea stării acestuia.

### 15.1 Acces

Pagina este afișată după `Start` atunci când:

- panoul a fost identificat;
- floor-ul este deja setat;
- nu este necesară o operație obligatorie înainte de intrarea în meniul de service.

### 15.2 Funcții

Pagina trebuie să ofere funcțiile definite în `svcbox_menu.drawio`:

- `EXIT SERVICE MODE`
- `FLASH`
- `FLOOR SET`

Textul din diagramă poate include și alte funcții, însă acestea nu trebuie implementate ca pagini sau fluxuri UI până când nu sunt definite explicit în structura meniului.

### 15.3 Exit Service Mode

La selectarea `EXIT SERVICE MODE`:

`Mode Selection → Start`

Nu este necesar un nou handshake pentru această revenire simplă la `Start`.

### 15.4 Flash

La selectarea `FLASH`:

`Mode Selection → Panel Type Selection`

De aici se continuă fluxul:

`Panel Type Selection → Select Firmware → Confirm & Flash → FLASHING → RECONNECT → Handshake / Panel Identification → Start`

Handshake-ul după flash este obligatoriu.

### 15.5 Floor Set

La selectarea `FLOOR SET`:

`Mode Selection → Current Floor Set`

De aici se continuă fluxul:

`Current Floor Set → SETTING FLOOR → RECONNECT → Handshake / Panel Identification → Start`

Handshake-ul după setarea floor-ului este obligatoriu.

### 15.6 Reguli

`Mode Selection` nu execută direct operațiile de comunicație, flash sau setare a floor-ului.

UI-ul transmite solicitarea către layerul Service/Application și gestionează numai:

- afișarea meniului;
- selectarea funcției;
- tranziția către pagina corespunzătoare;
- blocarea interacțiunilor în timpul operațiilor;
- revenirea la `Start` după finalizarea operației și reconectarea cu succes.

## 16. Communication

Pagina `Communication` este utilizată pentru testarea comunicațiilor panoului cu restul sistemului.

Testul nu reprezintă testarea comunicației dintre Service Box și panou.

Comunicațiile vizate sunt interfețele de comunicație utilizate de panou pentru legătura cu restul sistemului, de exemplu:

- RS485;
- Zigbee;
- alte interfețe de comunicație definite pentru panoul respectiv.

### 16.1 Elemente

Pagina conține:

- `TEST`;
- `RETURN`.

### 16.2 Communication Test

La selectarea `TEST` se lansează secvența de testare a comunicațiilor panoului.

Testul verifică funcționarea comunicațiilor dintre panou și restul sistemului, nu comunicația Service Box ↔ panou.

În timpul executării testului, UI trebuie să indice clar că testul este în desfășurare.

Starea butonului `TEST` poate fi schimbată temporar în:

`TESTING`

După finalizarea secvenței, butonul revine la:

`TEST`

UI nu trebuie să interpreteze sau să afișeze rezultatul testului ca pe un test al propriei comunicații cu panoul.

Execuția efectivă a testelor și accesul la interfețele de comunicație sunt responsabilitatea layerului Service/Application și/sau HAL corespunzător.

### 16.3 Return

La selectarea `RETURN`, utilizatorul revine la:

`Communication → Mode Selection`

### 16.4 Reguli

Pagina `Communication` nu trebuie să inițieze sau să reprezinte handshake-ul utilizat pentru identificarea panoului de către Service Box.

Handshake-ul Service Box ↔ panou este o funcție separată și face parte din fluxurile de identificare și reconectare.

`Communication Test` este destinat exclusiv verificării comunicațiilor panoului cu restul sistemului.

## 17. Display

Pagina `Display` este utilizată pentru testarea și reinițializarea display-urilor conectate la panoul elevatorului.

Pagina nu afișează rezultatul grafic al testului pe display-ul Service Box. Display-ul Service Box rămâne utilizat exclusiv pentru interfața UI și pentru afișarea stării operației.

### 17.1 Elemente

Pagina conține:

- `DISPLAY 1` — buton select/unselect;
- `DISPLAY 2` — buton select/unselect;
- `TEST` — buton select/unselect;
- `REINIT` — buton, momentan activ (`ON`).

### 17.2 Selectarea display-urilor

`DISPLAY 1` și `DISPLAY 2` sunt butoane de tip select/unselect.

Acestea stabilesc asupra căror display-uri externe se aplică operația următoare.

Starea butoanelor trebuie să indice vizual dacă display-ul respectiv este selectat.

Pentru panourile `CAB` sau `SIMPLEX`, unde este disponibil un singur display:

- `DISPLAY 1` este disponibil;
- `DISPLAY 2` este inactiv.

### 17.3 Test

Butonul `TEST` este de tip select/unselect.

În starea normală acesta afișează:

`TEST`

După inițierea operației, starea butonului se schimbă în:

`TESTING`

`TESTING` indică faptul că secvența de testare a display-ului sau display-urilor selectate este în desfășurare.

După terminarea secvenței, butonul revine la starea:

`TEST`

UI nu trebuie să afișeze pe ecranul Service Box conținutul sau rezultatul grafic al testului executat pe display-urile panoului.

### 17.4 Reinit

Butonul `REINIT` este momentan activ (`ON`).

La selectarea `REINIT`, se execută operația de reinițializare asupra display-ului sau display-urilor selectate.

În timpul operației, UI trebuie să indice faptul că reinițializarea este în desfășurare, fără a confunda această operație cu afișarea testului pe Service Box.

### 17.5 Selecția operației

Selecția `DISPLAY 1` / `DISPLAY 2` determină ținta operației.

`TEST` și `REINIT` determină operația executată asupra țintei selectate.

Exemple:

`DISPLAY 1 selected + TEST` → test display 1

`DISPLAY 2 selected + TEST` → test display 2

`DISPLAY 1 + DISPLAY 2 selected + TEST` → test ambele display-uri

`DISPLAY 1 selected + REINIT` → reinițializare display 1

`DISPLAY 1 + DISPLAY 2 selected + REINIT` → reinițializare ambele display-uri

### 17.6 Reguli

Pagina `Display` este un panou de comandă pentru display-urile externe ale panoului.

Service Box nu trebuie să utilizeze propriul display ca țintă pentru operațiile `TEST` sau `REINIT`.

UI-ul gestionează selecția, starea butoanelor și indicarea operației în curs. Execuția efectivă a comenzilor asupra display-urilor panoului este realizată de layerul hardware/service corespunzător.

## 18. MCU INFO

Pagina `MCU INFO` afișează informații tehnice despre microcontroller-ul și starea de funcționare a Service Box.

### 18.1 Elemente

Pagina conține:

- informații despre uptime;
- numărul de resetări;
- informații privind watchdog-ul (`WDT`);
- temperatura MCU;
- informații privind stack-ul;
- alte informații tehnice disponibile.

### 18.2 Return

Pagina conține butonul `RETURN`.

La selectarea `RETURN`, utilizatorul revine la:

`MCU INFO → Mode Selection`

### 18.3 Reguli

Informațiile afișate sunt destinate diagnosticării și service-ului.

Pagina `MCU INFO` este informativă și nu trebuie să modifice configurația sau starea de funcționare a Service Box.

Valorile trebuie obținute de la layerul hardware/system corespunzător; UI-ul este responsabil doar pentru afișarea acestora.

## 19. Reguli generale de interacțiune UI

### 19.1 Navigare

Navigarea între pagini trebuie să respecte strict fluxurile definite în `svcbox_menu.drawio`.

UI nu trebuie să introducă pagini, meniuri, butoane sau tranziții care nu sunt definite în structura aprobată.

### 19.2 Operații în desfășurare

În timpul unei operații care nu poate fi întreruptă în siguranță:

- comenzile incompatibile sunt dezactivate;
- navigarea către alte pagini este dezactivată;
- UI afișează clar starea operației în desfășurare;
- operația nu poate fi lansată repetat.

Exemple:

- `FLASHING` în timpul operației de flash;
- `SETTING FLOOR` în timpul setării floor-ului;
- `RECONNECT` în timpul reconectării și handshake-ului.

### 19.3 Starea informațiilor

UI trebuie să utilizeze numai informații confirmate și actualizate.

După operații care pot modifica starea panoului, informațiile vechi nu trebuie reutilizate pentru afișarea paginii `Start`.

În special, după:

- `FLASH`;
- `FLOOR SET`;

trebuie executat un nou handshake înainte de afișarea `Start`.

### 19.4 Separarea UI de hardware și service

UI gestionează:

- afișarea paginilor;
- elementele grafice;
- interacțiunea cu utilizatorul;
- stările și tranzițiile UI.

Accesul direct la hardware, comunicații, flash, SD card și alte operații de service trebuie realizat prin layerul corespunzător.

UI nu trebuie să implementeze direct protocoale de comunicație sau drivere hardware.

### 19.5 Afișarea stărilor

Stările operațiilor trebuie să fie vizibile și neambigue pentru utilizator.

Textele de stare trebuie să descrie operația efectiv executată, fără a sugera o altă operație.

De exemplu:

- `FLASHING` — operația de flash este în desfășurare;
- `SETTING FLOOR` — setarea floor-ului este în desfășurare;
- `RECONNECT` — operația anterioară s-a încheiat și Service Box execută reconectarea/handshake-ul.

### 19.6 Comportamentul la eroare

O eroare de comunicație, flash, reconectare sau altă operație de service nu trebuie să conducă la afișarea unor informații vechi ca fiind valide.

UI trebuie să afișeze o stare de eroare clară și să permită continuarea numai conform fluxului definit pentru operația respectivă.

Comportamentul specific pentru fiecare tip de eroare va fi definit în layerul Service/Application și în cerințele operației respective.

### 19.7 Caracter evolutiv al cerințelor și urmărirea implementării

`ui_requirements.md` este documentul de referință pentru cerințele funcționale și comportamentul UI.

Pe parcursul dezvoltării pot apărea cerințe noi, clarificări sau modificări ale cerințelor existente. Acestea trebuie documentate în `ui_requirements.md` și, atunci când necesită implementare sau verificare, trebuie reflectate și în `ui_todo.md`.

Starea implementării nu se marchează în `ui_requirements.md`.

`ui_todo.md` este utilizat separat pentru urmărirea:

- cerințelor care trebuie implementate;
- modificărilor care trebuie efectuate;
- testelor care trebuie realizate;
- verificărilor necesare;
- cerințelor finalizate.

O cerință nouă poate fi mai întâi descrisă și clarificată în `ui_requirements.md`, apoi introdusă în `ui_todo.md` ca element de lucru.

După implementare și validare, elementul corespunzător din `ui_todo.md` este marcat `DONE`.

Astfel, `ui_requirements.md` descrie permanent comportamentul dorit al sistemului, iar `ui_todo.md` urmărește progresul implementării fără a amesteca specificația cu starea proiectului.