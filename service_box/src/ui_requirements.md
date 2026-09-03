# Cerințe UI – Service Box

## 1. Scop

Definirea interfeței grafice pentru Service Box.

Interfața trebuie să fie simplă, profesională și potrivită pentru utilizarea
în activități de service pe teren.

Nu se adaugă funcționalități, pagini sau informații care nu sunt specificate
explicit în acest document.

Paginile și funcțiile suplimentare ale Service Box vor fi definite și
adăugate ulterior, pe măsură ce proiectul este dezvoltat.

Specificația UI este documentul de referință pentru deciziile privind
aspectul vizual și interacțiunea cu utilizatorul.


---

## 2. Display

- Rezoluție țintă: 240x320 pixeli.
- Orientare: portret.
- Toate platformele hardware utilizează aceeași interfață logică.
- Codul specific display-ului aparține HAL.
- Codul specific touchscreen-ului aparține HAL.
- Codul UI nu trebuie să conțină tratare specifică hardware pentru display
  sau touchscreen.

Platforme hardware:

1. Waveshare RP2350-Touch-LCD-2.8
2. GroundStudio Marble Pico + TFT 3.2"

Ambele platforme trebuie să ofere aceeași experiență de utilizare.


---

## 3. Secvența de pornire

Secvența de pornire a Service Box este:

    PORNIRE
       ↓
    VERIFICARE BATERIE
       ↓
    ECRAN START
       ↓
    utilizatorul apasă START
       ↓
    MENIUL PRINCIPAL

Ordinea de mai sus este obligatorie.

Ecranul existent de START se păstrează și nu se modifică prin introducerea
ecranului de verificare a bateriei.


---

## 4. Pagina Battery Check – Verificare baterie

Pagina Battery Check este afișată imediat după pornirea dispozitivului și
înaintea ecranului START.

Scopul acestei pagini este afișarea stării bateriei înainte ca utilizatorul
să poată accesa interfața Service Box.

### 4.1. Stare normală

Atunci când dispozitivul NU este în curs de încărcare, pagina afișează:

    BATTERY LEVEL

         XX %

Valoarea procentuală trebuie afișată clar și vizibil, în zona centrală
a ecranului.

Exemplu:

    BATTERY LEVEL

          87 %


### 4.2. Indicarea nivelului bateriei

Valoarea procentuală este reprezentată vizual prin culoarea valorii afișate.

Stările sunt:

- nivel scăzut → ROȘU
- nivel mediu → GALBEN
- nivel bun → VERDE

Pragurile numerice exacte pentru cele trei niveluri vor fi stabilite
ulterior.

Conversia tensiunii bateriei în procent nu aparține UI-ului.

UI-ul trebuie să primească de la nivelul hardware/aplicație:

- starea de încărcare;
- procentul bateriei.

Metoda efectivă de măsurare și conversia tensiune → procent sunt
dependente de hardware și nu trebuie implementate în codul UI.


### 4.3. Starea Charging

Dacă dispozitivul este în curs de încărcare, pagina nu afișează procentul
bateriei.

Se afișează:

    CHARGING

Pagina rămâne afișată în această stare.

În timpul încărcării:

- nu se execută trecerea automată către ecranul START;
- nu se aplică temporizarea de 5 secunde;
- pagina rămâne permanent afișată cât timp starea `CHARGING` este activă;
- nu este necesară interacțiunea utilizatorului.

Atunci când încărcarea se încheie, dispozitivul revine la comportamentul
normal al paginii Battery Check.


### 4.4. Temporizarea Battery Check

Dacă dispozitivul NU este în stare `CHARGING`:

1. se citește starea bateriei;
2. se afișează nivelul bateriei în procente;
3. pagina rămâne afișată timp de 5 secunde;
4. după expirarea celor 5 secunde se trece automat la ecranul START
   existent.

Cele 5 secunde reprezintă timpul minim de afișare a paginii Battery Check
în regim normal.


### 4.5. Interacțiunea utilizatorului

Pagina Battery Check este o pagină informativă.

Nu conține:

- butoane;
- comenzi;
- meniuri;
- gesturi;
- controale touchscreen.

Utilizatorul nu trebuie să efectueze nicio acțiune pentru trecerea la
ecranul START în regim normal.


### 4.6. Informații care NU trebuie afișate

Pagina Battery Check nu trebuie să afișeze:

- informații CPU;
- informații display;
- informații touchscreen;
- informații RS485;
- informații WDT;
- contoare de pachete;
- informații de diagnostic;
- informații SD;
- bare de progres;
- informații suplimentare care nu au legătură cu bateria.


---

## 5. Ecranul START

Ecranul START este ecranul principal de prezentare al produsului și
reprezintă pagina existentă înainte de introducerea Battery Check.

Conținut:

- SERVICE BOX
- PANOURI ASCENSOARE
- versiunea firmware;
- buton START.

Informații SD opționale:

- starea cardului SD;
- spațiul liber / spațiul total.

Exemplu:

    SERVICE BOX

    PANOURI ASCENSOARE

         v1.0.0

        [ START ]

    SD  ● 742 MB / 1.8 GB FREE


Nu se afișează pe ecranul START:

- starea CPU;
- starea display-ului;
- starea touchscreen-ului;
- starea RS485;
- starea WDT;
- contoare de pachete;
- informații de diagnostic;
- „Starting...”;
- bare de progres.

Service Box trebuie să rămână utilizabil și în absența cardului SD.


---

## 6. Touchscreen

- Se utilizează zone tactile mari și clar separate.
- Zona tactilă poate fi puțin mai mare decât butonul vizibil.
- Se evită controalele mici.
- Se evită gesturile inutile.
- Se preferă interacțiunea simplă prin apăsare (tap).
- Coordonatele touchscreen-ului sunt furnizate de HAL.

Pagina Battery Check nu utilizează interacțiune touchscreen.


---

## 7. Stil vizual

Stilul interfeței:

- industrial;
- curat;
- tehnic;
- profesional;
- sobru.

Se evită:

- elementele decorative inutile;
- utilizarea excesivă a culorilor;
- aspectul de interfață pentru smartphone;
- animațiile complexe;
- cantitatea excesivă de informații;
- pictogramele inutile.

Ierarhia vizuală trebuie obținută prin:

- dimensiune;
- spațiere;
- chenare;
- contrast;
- indicatori simpli de stare.

Culorile pot fi utilizate pentru indicarea unor stări importante, cum este
nivelul bateriei.


---

## 8. Branding

Numele principal:

    SERVICE BOX

Descrierea produsului:

    PANOURI ASCENSOARE


---

## 9. Arhitectura UI

UI-ul trebuie să fie independent de hardware.

Structura recomandată:

    Aplicație / Logică Service
              ↓
           Logică UI
              ↓
        UI Renderer
              ↓
             HAL
           ↙     ↘
       Display    Touch


Logica UI nu trebuie duplicată pentru diferite platforme hardware.

Nu se introduc blocuri `#ifdef` specifice hardware-ului în codul UI decât
dacă acest lucru este absolut necesar.

Citirea bateriei și identificarea stării `CHARGING` aparțin nivelului
hardware/aplicație, nu rendererului UI.

UI-ul primește datele necesare și decide doar modul în care acestea sunt
afișate.


---

## 10. Reguli pentru dezvoltare

Înainte de implementarea unei pagini noi:

1. Se verifică mai întâi codul existent al Service Box.
2. Se reutilizează funcționalitatea și interfețele existente.
3. Nu se inventează funcționalități noi fără specificarea lor.
4. Nu se modifică funcționalitatea HAL decât dacă este necesar.
5. UI-ul trebuie păstrat modular.
6. Definițiile paginilor și tratarea touchscreen-ului trebuie să rămână
   ușor de modificat.
7. Funcționalitatea specifică hardware trebuie menținută în HAL sau în
   nivelul hardware/application layer.
8. Paginile suplimentare se implementează numai după ce cerințele lor sunt
   definite în acest document.


---

## 11. Extinderea ulterioară a interfeței

În această etapă sunt definite explicit:

1. secvența de pornire;
2. pagina Battery Check;
3. ecranul START;
4. trecerea către MENIUL PRINCIPAL.

Paginile care urmează după MENIUL PRINCIPAL NU sunt încă definite în acest
document.

Acestea vor fi specificate și adăugate ulterior.

Nu se implementează anticipat pagini de:

- diagnostic;
- configurare;
- programare;
- SD;
- RS485;
- panouri;
- firmware;
- alte funcții de service,

decât după definirea explicită a cerințelor acestora.

Structura UI trebuie să permită adăugarea ulterioară a acestor pagini fără
modificarea inutilă a paginilor deja implementate.


---

## 12. Principiu general

Implementarea UI trebuie să urmeze principiul:

    SPECIFICAȚIE
         ↓
    LOGICĂ UI
         ↓
    RENDERER
         ↓
       HAL
         ↓
      HARDWARE

Specificația acestui document este autoritativă pentru aspectul vizual,
fluxul paginilor și interacțiunea cu utilizatorul.

Orice funcționalitate sau pagină nouă trebuie definită în acest document
înainte de implementare.