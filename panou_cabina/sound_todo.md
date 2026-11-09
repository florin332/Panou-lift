# Sound TODO

## Decizii stabilite

- Sunetele nu vor mai fi hardcodate/incluse in firmware; vor fi citite de pe cardul SD onboard al Marble Pico.
- Format audio: WAV cu PCM, mono, 16 kHz; iesirea I2S va ramane la 16 kHz, aceeasi rata ca fisierele, pentru a evita resampling-ul daca decoderul permite.
- Format PCM tinta: 16-bit semnat; semnalul mono este duplicat pe canalele L/R I2S. I2S va folosi PIO + DMA.
- Evenimentele audio raman definite explicit in firmware. Fisierele de pe SD au ID-uri de sunet independente `01`-`31`, iar interfata web poate reasigna evenimentele la orice sunet valid; mai multe evenimente pot folosi acelasi sunet.
- Cardul SD este instalat intern si nu este accesibil din exterior; lipsa cardului nu este un caz normal care necesita tratare dedicata.
- In firmware se pastreaza trei fallback-uri fixe, asociate evenimentelor alarma, suprasarcina si oprire. Daca sunetul selectat lipseste, este corupt sau nu poate fi incarcat, evenimentul foloseste fallback-ul sau fix din firmware. Schimbarea continutului fallback-urilor necesita modificarea sursei, recompilarea si rescrierea firmware-ului; web-ul nu le poate modifica.
- La pornire, dupa indexarea si validarea fisierelor SD, sunetele selectate sunt incarcate integral intr-un cache din RAM sau flash si redate din cache, nu prin citiri SD la fiecare eveniment.
- Dupa upload sau stergere din interfata web, cardul se reindexeaza si cache-ul se actualizeaza, astfel incat noile sunete sa poata fi folosite fara restart/recompilare.
- Cardul SD si LCD-ul impart SPI0; in arhitectura tinta, numai Core 1 acceseaza SPI0, iar Core 0 nu acceseaza SD/LCD.
- Arhitectura tinta: Core 0 gestioneaza audio, evenimentele ALARM/OVERLOAD/RESET si W5500 pe SPI1; Core 1 gestioneaza SD si LCD pe SPI0, indexeaza cardul si actualizeaza cache-ul audio.
- Tinta de latenta pentru pornirea sunetului la eveniment este sub 1 ms, folosind date preincarcate in RAM. Este o tinta de masurat pe hardware, nu o performanta confirmata.
- La pornire si dupa modificari web, Core 1 valideaza antetele RIFF/WAVE si CRC-ul, apoi incarca integral in cache sunetele selectate. La eveniment, Core 0 reda din cache fara acces SD.
- Sunetele din cache se publica spre Core 0 numai dupa ce sunt incarcate si validate complet. Trebuie definita o comutare sigura daca se actualizeaza un sunet in timp ce acesta este redat.
- Cache-ul integral in RAM sau flash scoate latenta SD din calea pornirii si previne underrun-urile cauzate de citiri SD in timpul redarii. SPI0 ramane partajat la indexare/incarcare si intre accesul SD si LCD.
- Fisierele de pe card vor avea ID numeric obligatoriu `01`-`31`, urmat de `_`, un nume descriptiv si extensia lowercase `.wav` (de exemplu `01_chime.wav`). ID-ul identifica sunetul, nu evenimentul; este permis un singur fisier per ID. Numele vor avea maximum 31 de caractere si vor folosi numai ASCII lowercase, cifre, `_` si `-`.
- Evenimentele audio vor fi definite explicit in cod, iar fiecarui eveniment i se va putea atribui din interfata web un ID de sunet `01`-`31`. Maparea va fi persistenta; fallback-urile firmware raman fixe pe evenimente si independente de aceasta mapare.
- Pot fi adaugate ulterior evenimente si fisiere audio noi; implementarea trebuie sa permita extinderea listei si a maparilor.
- Scrierea si stergerea fisierelor audio sunt permise numai prin interfata web sau direct de pe PC.
- Service box-ul nu va avea functii/comenzi de scriere sau stergere a fisierelor audio.
- Atat interfata web, cat si service box-ul vor putea comanda redarea unui sunet.
- Comanda de redare din interfata web va functiona si cand panoul nu este in service mode.
- Service box-ul va avea o comanda de tip `mb_xx`, cu argument pentru redarea unui sunet in service mode. Comanda web de redare va fi separata.

## Card SD si pini

Pinii comunicati pentru cititorul onboard:

| Semnal | GPIO |
| --- | ---: |
| DT3 / CS | GP17 |
| CMD / MOSI | GP19 |
| CLK / SCK | GP18 |
| DT0 / MISO | GP16 |

GP18 si GP19 sunt deja folositi de LCD pe SPI0. Conexiunile indica o magistrala SPI partajata, cu selectii separate pentru card (GP17) si LCD (GP22). De verificat la implementare modul de lucru al cititorului si biblioteca folosita; accesul la dispozitive trebuie coordonat astfel incat transferurile SD si LCD sa nu se suprapuna.

W5500 ramane pe SPI1 (GP12-GP15), separat de cardul SD si LCD. Tinta este SD la maximum 20 MHz si alimentat la 3.3 V, fara conversie de nivel care ar putea bloca MISO.

## Starea actuala

- Driverul `Sound` reda cele trei WAV-uri incorporate in flash prin API-ul Arduino-Pico `I2S`; nu exista inca citire de pe SD sau cache audio incarcat de pe card.
- La initializare sunt validate WAV-uri PCM mono, 16 kHz, 16-bit. Semnalul mono este duplicat pe canalele L/R I2S.
- Iesirea audio este I2S catre MAX98357A, pe GP8 (BCLK), GP9 (LRCLK) si GP10 (DATA).
- Rata de redare este 16 kHz, gain-ul foloseste setarile persistente existente, iar coada I2S este configurata la 8 x 256 frame-uri (8192 bytes).
- Interfata `alarm()`, `overload()` si `liftStopped()` este folosita pentru cele trei sunete existente.
- Un apel de redare nou inlocuieste clipul activ si incearca sa scrie noul clip in spatiul disponibil. Datele deja puse in coada I2S continua sa se redea, dar nu exista o coada de clipuri; partea veche care nu a fost deja pusa in coada poate fi abandonata. Politica pentru evenimente simultane ramane de stabilit.
- Separarea pe nuclee este implementata pentru componentele existente: Core 0 detine modelul, evenimentele, audio si transportul USB/W5500; Core 1 detine LCD-ul si prezentarea. Accesul SD pe Core 1 si protocolul de publicare a cache-ului audio nu sunt implementate.
- Build-ul si upload-ul `marble` raportate au avut exit code 0. Pornirea sub 1 ms, absenta underrun-urilor si actualizarea cache-ului nu sunt validate prin masuratori hardware.

## Ordinea pasilor

1. Verifica pe schema si hardware configuratia electrica si de bus: SD si LCD pe SPI0 cu CS separat, SD la maximum 20 MHz, LCD la 40 MHz, SD alimentat la 3.3 V si ambele CS inactive inainte de initializare. Codul curent seteaza LCD-ul la 8 MHz si nu initializeaza inca SD-ul; confirma frecventele tinta si functionarea magistralei partajate inainte de a le declara validate.
2. Stabileste contractul fisierelor: ID-uri de sunet `01`-`31` independente de evenimente, nume lowercase cu formatul `<ID>_<nume>.wav` si maximum 31 de caractere, WAV PCM mono 16 kHz/16-bit cu reguli stricte de validare si CRC-32 (polinom `0xEDB88320`) comparat cu o valoare de referinta de incredere. Defineste unde se pastreaza referinta CRC si valideaza fallback-urile fixe din firmware pentru alarma, suprasarcina si oprire; schimbarea lor se face numai prin recompilare si rescriere firmware. Stabileste maparea persistenta eveniment-sunet configurabila din web.
3. Extinde separarea existenta: Core 0 gestioneaza evenimente/audio/W5500, iar Core 1 trebuie sa preia si accesul SD. Defineste protocolul de evenimente si sincronizarea cache-ului audio intre nuclee.
4. Initializeaza si valideaza SPI0 partajat si accesul la SD/LCD exclusiv din Core 1; masoara indexarea/incarcarea SD in timp ce LCD-ul este folosit.
5. Implementeaza catalogul de sunete: scanare la boot, validare RIFF/WAVE si CRC, incarcare integrala in cache a fisierelor selectate si fallback-urile din flash pentru cele trei sunete principale.
6. Masoara dimensiunea totala a sunetelor si RAM-ul utilizabil; decide daca se incarca toate sunetele sau doar cele trei principale si daca se foloseste cache RAM sau flash.
7. Audio-ul local la 16 kHz prin API-ul Arduino-Pico `I2S` si coada de 8 x 256 frame-uri sunt implementate. Ramane de integrare/verificare ca backend-ul foloseste PIO + DMA conform tintei; Core 0 trebuie sa redea din cache-ul validat, fara citiri SD in timpul redarii.
8. Implementeaza maparea eveniment-sunet configurabila din web si persistenta ei peste restart; evenimentele raman definite explicit in cod.
9. Adauga comenzile de redare distincte: service box (`mb_xx` cu argument, numai in service mode) si web (disponibila si in afara service mode). Service box-ul nu primeste comenzi de scriere/stergere.
10. Adauga incarcarea/stergerea fisierelor prin web si procedura de actualizare de pe PC; sincronizeaza metadatele CRC, reindexeaza cardul si actualizeaza cache-ul fara sa invalideze sunetul redat curent.
11. Integreaza LCD-ul cu prioritate sub audio si deseneaza doar zonele schimbate; nu conditiona desenarea de un prag de date audio citite de pe SD.
12. Valideaza pe hardware pornirea sub 1 ms din cache, absenta underrun-urilor, actualizarea live a cache-ului, fallback-ul la fisier corupt/CRC invalid, maparea persistenta si restrictiile comenzilor web/service box.

## De stabilit / verificat la implementare

- Modelul curent este cache integral: redarea nu citeste de pe SD. Masoara latenta SD doar la indexare/incarcare/reindexare; bufferul DMA I2S ramane separat de bufferul de cache.
- Numele exacte ale fisierelor, extensiile si locatia lor pe card.
- Validarea formatului WAV acceptat, inclusiv adancimea de biti si codec-ul PCM; confirmarea ca decoderul reda la 16 kHz fara conversie de rata.
- Comportamentul pentru fisier lipsa, corupt sau cu eroare de citire: fallback la copia din flash pentru sunetele principale; stabilirea comportamentului pentru sunetele fara copie in flash.
- Stabilirea verificarii integritatii fisierelor audio cu CRC. CRC-ul calculat trebuie comparat cu o valoare de referinta de incredere; calcularea CRC-ului fara valoare asteptata nu detecteaza coruperea.
- Stabilirea CRC-ului de referinta si a locului unde se pastreaza. Validarea se face la indexare/incarcare, nu la fiecare redare; upload-ul web trebuie sa calculeze sau sa actualizeze metadatele CRC.
- Masurarea dimensiunii fisierelor si a RAM-ului utilizabil; stabilirea daca se incarca toate sunetele sau doar cele principale si daca se foloseste cache RAM sau flash. Cele trei copii fallback din firmware raman fixe.
- Comportamentul curent la evenimente audio simultane: un clip nou inlocuieste sursa activa, datele vechi deja puse in coada continua sa se redea, iar clipul vechi nu este pastrat integral. Nu exista coada explicita de clipuri; stabileste daca acest comportament ramane contractul dorit sau daca se adopta intrerupere/coada/prioritati.
- Masurarea latentei pornirii sub 1 ms din cache. Masoara separat indexarea/reincarcarea SD si efectul acestora asupra actualizarii LCD; redarea nu citeste de pe card.
- Separarea pe nuclee este implementata prin `setup1()`/`loop1()`: Core 0 detine modelul, evenimentele, audio si transportul USB/W5500; Core 1 detine LCD-ul si prezentarea. `SharedPanel` ramane publicat de Core 0 si citit cu seqlock pe Core 1. Comenzile service trec Core 0 -> Core 1, iar raspunsurile sunt puse intr-o coada Core 1 -> Core 0, astfel incat transportul sa ramana exclusiv pe Core 0.
- Protocolul de schimb al cache-ului audio ramane de implementat odata cu catalogul SD: Core 1 construieste si valideaza un bank imuabil, apoi publica pointerul si generatia; Core 0 comuta la bank-ul nou doar la o limita sigura a redarii si confirma generatia. Core 1 nu modifica si nu reutilizeaza bank-ul vechi pana la confirmare; redarea nu citeste niciodata de pe SD.
- Fluxul web pentru incarcare si stergere, inclusiv protectia operatiilor cat timp un fisier este folosit la redare.
- Accesul direct de pe PC si procedura recomandata pentru actualizarea fisierelor pe card.
- Stabilirea modului de salvare/persistenta a maparii eveniment-sunet configurate din web.
- Stabilirea ID-urilor si argumentelor exacte pentru comenzile distincte de redare din service box (`mb_xx`) si din interfata web.
- Extinderea maparii eveniment-audio pentru evenimentele viitoare.
