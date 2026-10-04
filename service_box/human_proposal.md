Urmează să implementezi modulul `BatteryManagement` pentru proiectul "Panou-lift / service_box". Dispozitivul rulează pe o placă GroundStudio Marble Pico (RP2040). Acest modul hardware specific va fi implementat izolat pe branch-ul hardware dedicat ('marble') și NU în 'main'.

# 1. CONTEXT ARHITECTURAL & FLUX FIZIC
Dispozitivul funcționează în două moduri mari:
- MOD HOST (Funcționare normală pe teren): Alimentat exclusiv din acumulatorul propriu. Mufa USB-C este folosită pentru a comunica ca USB Host cu o altă placă Pico (panoul de lift), pentru a-i modifica setarile sau pentru a o trece în mod UF2 și a-i scrie un firmware preîncărcat de pe cardul SD. Panoul de lift are alimentarea sa proprie, permanentă.
- MOD NORMAL / MAINTENANCE (În atelier): Funcționează cu bateria scoasă, alimentat din USB-ul laptopului pentru programare/teste. În acest mod, un STUB software pre-implementat preia controlul în locul ADC-ului fizic pentru a simula stările bateriei.

---

# 2. SPECIFICAȚII HARDWARE 

## 2.1. GroundStudio Marble Pico
Conform schemei electrice a plăcii Marble Pico, maparea și logica pinilor este următoarea:
- GP24 (Digital Input) = `USB_DETECT / POWER_PATH`. HIGH (1) când USB-C (5V) este conectat și tranzistorii izolează bateria. LOW (0) când USB este deconectat (regim de teren).
- GP23 (Digital Input) = `BAT_ACTIVE_STATUS`. HIGH (1) când bateria este conectata și alimentează activ placa în regim Host autonom. LOW (0) când sistemul este alimentat prin USB (încărcătorul TP4065 e activ, terminat, sau bateria e scoasă).
- GP29 (Analog Input, Canal ADC 3) = `VBUS_ADC`. Măsoară tensiunea de pe magistrala VBUS printr-un divizor rezistiv intern (/3), conform schemei electrice.
- GP28 (Analog, ADC2) = `BAT_VOLTAGE_ADC`. Pin complementar, legat extern printr-un divizor rezistiv /3 direct la mufa acumulatorului. Folosit pentru măsurarea nivelului bateriei numai în timpul încărcării.


## 2.2. Waveshare - RP2350
Această placă folosește noul microcontroler RP2350 și dispune de un circuit activ de power management, ceea ce elimină necesitatea pinilor complementari sau a modificărilor hardware. Citirea bateriei pe USB funcționează nativ.
Mapare pini:
- GP26 (Digital Output): BSP_BAT_EN_PIN. Controlează electronic pornirea/oprirea circuitului de măsurare pentru a economisi energie.
- GP25 (Digital Input): BSP_BAT_KEY_PIN. Monitorizează butonul fizic de Power al cutiei de service.
- GP27 (Analog, ADC1): Măsoară tensiunea acumulatorului printr-un divizor rezistiv /3 integrat.
Logica de Funcționare (State Machine):
- Algoritm de filtrare anti-zgomot incorporat: Pentru a opri fluctuațiile flag-urilor din UI produse de consumul cardului SD, driverul efectuează 9 citiri consecutive, le ordonează prin Bubble Sort, elimină extremele (vârfurile de zgomot) și face media celor 7 valori rămase.
- Mod HOST (Pe teren): Codul rulează bsp_battery_enabled(true), efectuează citirea filtrată pe ADC1 (GP27) și convertește valoarea în volți reali (ADC * (3.3/4096) * 3.0). Raportează bat_pow și nivelul real.
- Mod Pe USB (La încărcat / Atelier): Deoarece divizorul de pe Waveshare este plasat înainte de circuitele de izolare, pinul GP27 măsoară corect tensiunea bateriei chiar și cu cablul USB conectat.Dacă tensiunea pe GP27 crește spre 4.15V( Dacă tensiunea pe GP27 crește spre 4.15V, raportează dinamic stările bat_chg -> chg_full; Dacă se detectează o tensiune constantă de ≈ 0V (acumulator deconectat din mufă), se ridică flag-ul no_bat. La fel ca la varianta Marble, hardware-ul real este decuplat, se pornește simulatorul BatteryStub și se activează logica de persistență a stărilor de test în EEPROM).

---

# 3. CERINȚE DE IMPLEMENTARE SOFTWARE (C++)

Modulul `BatteryManagement` trebuie să proceseze acești pini (sau Stub-ul în Modul Normal) și să expună către interfața grafică (`src/ui`) și aplicația principală (`main.cpp`) EXCLUSIV următoarele 8 informații/flag-uri discrete, care trebuie să fie mutual exclusive acolo unde este cazul (de exemplu la niveluri):

1. `usb_pow` (bool) -> Alimentat din USB (Condiție: GP24 == 1) / Sursa Hardware: GP24 == HIGH. / Sursa Stub: Activat manual din interfață/comenzi seriale.
2. `bat_pow` (bool) -> Alimentat din baterie / Mod Host (Condiție: GP24 == 0 și GP23 == 1) / Sursa Hardware: GP24 == LOW. Notă: Doar în această stare se citește GP29 (ADC3) pentru a trimite flag-urile de nivel (bat_10, bat_40, bat_100) către UI.
3. `bat_chg` (bool) -> În curs de încărcare / Sursa Hardware: Devine un default conservator. Dacă usb_pow == true, modulul raportează automat bat_chg = true (deoarece fizic TP4065 va încerca mereu să încarce bateria dacă este prezentă).
4. `chg_full` (bool) -> Încărcare completă (Condiție: GP24 == 1, GP23 == 0 și Tensiune >= 4.15V stabil)
5. `no_bat` (bool) -> Sursa Hardware:Dezactivat (Mereu false în regim normal) / 
6. `bat_100` (bool) -> Nivel baterie 100% (Tensiune > 3.85V din ADC sau Stub)
7. `bat_40` (bool) -> Nivel baterie 40% (Platou nominal: 3.55V < Tensiune <= 3.85V din ADC sau Stub)
8. `bat_10` (bool) -> Nivel baterie 10% / Critic (Tensiune <= 3.55V din ADC sau Stub) -> Va fi folosit de UI pentru blocarea sigură a procesului de flashing USB Host.

---

# 4. Analiza Inconcordanțelor și Schimbările Necesare 
1. Schimbarea totală de paradigmă: Înlocuitor Temporar vs. Mod de Lucru Permanent (no_bat)
   Cum e gândit stub-ul acum: este un "înlocuitor temporar până e gata hardware-ul", iar la integrarea reală "se șterge includerea de BatteryStub.h și se creează instanța reală".Cum trebuie să fie în realitate (service_box): Stub-ul nu trebuie șters! Dispozitivul va funcționa pe bancul de lucru în Modul Normal (cu bateria scoasă), driverul real din branch-ul marble va detecta hardware lipsa acumulatorului (no_bat devine true) și va activa automat acest Stub în mod permanent pe producție. Prin urmare, ambele componente vor coexista în branch-ul hardware, iar managerul va comuta dinamic între ele.

2. Discrepanța dintre Stările Stub-ului și cele 8 Flag-uri Cerute
   Stub-ul actual lucrează cu un enum simplificat de stări specifice UI-ului de boot (CRITICAL, LOW, NORMAL, CHARGING). Modulul final de BatteryManagement trebuie însă să transmită cele 8 flag-uri exacte (sau o structură echivalentă) pentru a acoperi logica de USB Host a cutiei de service:
   Flag-uri discrete | Ce livrează Stub-ul acum | Cum le aliniem în Arhitectură 
    - usb_pow        | Nu are echivalent        | Devine true în stările CHARGING și în Modul Normal (fără baterie).
    - bat_pow        | Nu are echivalent        | Devine true în stările CRITICAL, LOW, NORMAL (când rulează pe teren). 
    - bat_chg        |  CHARGING                | Corespunde perfect stării în care se injectează curent.
    - chg_full       | Nu are echivalent        | Trebuie adăugat ca flag dedicat când nivelul e 100% și USB-ul e cuplat.
    - no_bat         | Nu are echivalent        | Este starea cheie! Când pornește în State 4: MAINTENANCE, stub-ul raportează acest flag
    - bat_100        | NORMAL (sau 80%/100%)    |Se activează când nivelul simulat/real depășește pragul de 3.85V.
    - bat_40         |NORMAL (sau 40%/60%)      |Se activează pe platoul simulat/real de mijloc (3.55V - 3.85V).
    - bat_10         | CRITICAL / LOW           | Se activează sub 3.55V. Oprește secvența de Flash Firmware prin USB Host.

3. Logica de Persistență în EEPROM
   - În stub-ul actual: Nivelul și starea de încărcare supraviețuiesc repornirilor prin salvarea în EEPROM.
   - Impactul pe hardware real: Când dispozitivul este pe teren (Modul Host), această persistență trebuie ignorată total, deoarece citirea fizică de pe      pinul analogic ADC29 (VBUS_ADC) are prioritate absolută. Persistența din EEPROM rămâne utilă doar în Modul Normal/Atelier, pentru ca stub-ul să își amintească ce valoare se testa înainte de reset.

---

# 5.. Soluția Arhitecturală Finală
    
Pentru a păstra curățenia interfeței IBatteryProvider.h și a permite UI-ului (BatteryCheckScreen) să funcționeze identic și pe teren și în atelier, clasa de bază abstractă nu trebuie să returneze volți sau procente brute, ci trebuie să expună metode care mapă exact cele 8 flag-uri sau stările echivalente.Arhitectura va folosi un Model de Compunere (Wrapper):
- IBatteryProvider.h rămâne neschimbat ca interfață în src/battery.
- În branch-ul marble, creezi clasa MarbleBatteryManager care moștenește IBatteryProvider.În interiorul MarbleBatteryManager, instanțiezi atât driverul  hardware real (care citește pinii GP23, GP24, ADC29), cât și o instanță de BatteryStub.

# 6. 📊 Matricea de Comportament Unificată (`IBatteryProvider`)

Acest tabel definește comportamentul modulului `BatteryManagement` pentru ambele branch-uri hardware (`marble` și `waveshare`) în raport cu flag-urile transmise către interfața grafică (`src/ui`).

| Regim de Lucru | Mod Conexiune | `usb_pow` | `bat_pow` | `bat_chg` | `chg_full` | `no_bat` | Sursă Nivel Baterie | Reacție UI (`BatteryCheckScreen`) |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :--- | :--- |
| **HOST** | Pe teren / În lift<br>(Fără USB extern) | `0` | `1` | `0` | `0` | `0` | **Hardware Real**<br>• Marble: ADC3 (GP29)<br>• Waveshare: ADC1 (GP27) | **Funcționare Nominală:**<br>• Permite START dacă nivelul este `bat_40` / `bat_100`.<br>• **Blochează START forțat** dacă scade la `bat_10`. |
| **CHARGING** | Alimentator cuplat<br>+ Baterie prezentă | `1` | `0` | `1` | `0` | `0` | **Hardware Real**<br>• Marble: ADC2 (GP28)<br>• Waveshare: ADC1 (GP27) | **Mod Încărcare:**<br>• Afișează animația de încărcare pe ecran.<br>• Nu avansează automat după timeout, dar permite apăsarea START. |
| **FULL** | Încărcare terminată<br>(Tensiune ≥ 4.15V) | `1` | `0` | `0` | `1` | `0` | **Hardware Real**<br>• Marble: ADC2 (GP28)<br>• Waveshare: ADC1 (GP27) | **Baterie Plină:**<br>• Afișează pictograma de baterie 100% statică (fără animație de fulger). |
| **NORMAL /<br>MAINTENANCE** | În atelier / Pe masă<br>(Fără baterie) | `1` | `0` | `0` | `0` | `1` | **Software STUB**<br>• Deconectează ADC fizic.<br>• Ascultă REPL (ex: `bat 10`). | **Mod Depanare / Laborator:**<br>• Activează parserul serial din `BatteryStub.cpp`.<br>• Reacționează grafic instant la profilele injectate. |

---

## 📌 Note Arhitecturale pentru Implementare:

1. **Filtrarea software obligatorie (Anti-Zgomot SD Card):**
   - Pe ambele plăci, eșantioanele brute (set de 9 citiri consecutive) trebuie trecute prin filtrul median modificat înainte de evaluarea pragurilor logice.
   - Algoritm: Sortare crescătoare (*Bubble Sort*) -> Eliminare extreme (prima și ultima valoare) -> Medie aritmetică pe cele 7 eșantioane rămase la mijloc.

2. **Praguri Logice de Tensiune (Mapped pe 0-100%):**
   - **`bat_100`**: Tensiune > `3.85 V` (Treimea superioară).
   - **`bat_40`**: `3.55 V` < Tensiune ≤ `3.85 V` (Platoul nominal de descărcare LiPo).
   - **`bat_10`**: Tensiune ≤ `3.55 V` (Nivel critic / Avertizare descărcare).

3. **Prioritatea Modului Normal / Atelier:**
   - Dacă se confirmă condiția `no_bat` (tensiune reziduală ≈ 0V pe pinul dedicat de citire a celulei), clasa manager oprește automat interogarea hardware a ADC-ului și pasează controlul către instanța `BatteryStub`. Stările de test (level și charging) sunt persistate în EEPROM pentru a supraviețui repornirilor.

4.  Scrie codul în C++ curat, modular, respectând structura din `src/battery` și integrându-te cu `src/bsp`. Nu polua `main.cpp` cu 
    calcule matematice de voltaj.
5.  Generează structura clasei `BatteryManagement` (header și implementare), definind clar interfața publică prin care UI-ul poate citi 
    aceste 8 flag-uri.


