# Sound TODO

## Decizii stabilite

- Sunetele nu vor mai fi hardcodate/incluse in firmware; vor fi citite de pe cardul SD onboard al Marble Pico.
- Format audio: WAV cu PCM, mono, 16 kHz; iesirea I2S va ramane la 16 kHz, aceeasi rata ca fisierele, pentru a evita resampling-ul daca decoderul permite.
- Format PCM tinta: 16-bit semnat; semnalul mono este duplicat pe canalele L/R I2S. I2S va folosi PIO + DMA.
- Se pastreaza momentan cele trei evenimente audio existente: alarma, suprasarcina si oprire. Denumirile fisierelor vor ramane aceleasi ca cele folosite pentru sunetele curente; continutul audio de pe card poate fi diferit de cel incorporat acum in flash.
- Cardul SD este instalat intern si nu este accesibil din exterior; lipsa cardului nu este un caz normal care necesita tratare dedicata.
- In firmware se pastreaza trei copii de siguranta, cate una pentru fiecare sunet principal: alarma, suprasarcina si oprire. Daca varianta SD lipseste, este corupta sau nu poate fi incarcata, se foloseste copia din flash.
- La pornire, dupa indexarea si validarea fisierelor SD, sunetele selectate sunt incarcate integral intr-un cache din RAM sau flash si redate din cache, nu prin citiri SD la fiecare eveniment.
- Dupa upload sau stergere din interfata web, cardul se reindexeaza si cache-ul se actualizeaza, astfel incat noile sunete sa poata fi folosite fara restart/recompilare.
- Cardul SD si LCD-ul impart SPI0; in arhitectura tinta, numai Core 1 acceseaza SPI0, iar Core 0 nu acceseaza SD/LCD.
- Arhitectura tinta: Core 0 gestioneaza audio, evenimentele ALARM/OVERLOAD/RESET si W5500 pe SPI1; Core 1 gestioneaza SD si LCD pe SPI0, indexeaza cardul si actualizeaza cache-ul audio.
- Tinta de latenta pentru pornirea sunetului la eveniment este sub 1 ms, folosind date preincarcate in RAM. Este o tinta de masurat pe hardware, nu o performanta confirmata.
- La pornire si dupa modificari web, Core 1 valideaza antetele RIFF/WAVE si CRC-ul, apoi incarca integral in cache sunetele selectate. La eveniment, Core 0 reda din cache fara acces SD.
- Sunetele din cache se publica spre Core 0 numai dupa ce sunt incarcate si validate complet. Trebuie definita o comutare sigura daca se actualizeaza un sunet in timp ce acesta este redat.
- Cache-ul integral in RAM sau flash scoate latenta SD din calea pornirii si previne underrun-urile cauzate de citiri SD in timpul redarii. SPI0 ramane partajat la indexare/incarcare si intre accesul SD si LCD.
- Fisierele de pe card vor avea prefix numeric de doua cifre pentru identificare rapida (exemple: `01_alarma.wav`, `02_overload.wav`); structura exacta a numelor si maparea ID-urilor raman de stabilit.
- Evenimentele audio vor fi definite explicit in cod, iar fiecarui eveniment i se va putea atribui un anumit sunet/fisier audio din interfata web.
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

- Driverul `Sound` reda in prezent WAV-uri incorporate in flash prin `AudioFileSourcePROGMEM` si `AudioGeneratorWAV`.
- Iesirea audio este I2S catre MAX98357A, pe GP8 (BCLK), GP9 (LRCLK) si GP10 (DATA).
- Rata I2S curenta este 16 kHz, iar gain-ul este gestionat prin setarile persistente existente. Se pastreaza iesirea I2S la 16 kHz pentru fisierele WAV de 16 kHz.
- Interfata actuala expune `alarm()`, `overload()` si `liftStopped()`; acestea trebuie sa ramana asociate celor trei evenimente de mai sus.
- Reper build curent, cu cele trei sunete si o imagine in flash: 557752 din 8384512 bytes (6.7%). Aceasta este utilizarea totala raportata pentru firmware, nu dimensiunea individuala a resurselor audio sau imaginii.

## Ordinea pasilor

1. Confirma configuratia electrica si de bus: SD si LCD pe SPI0 cu CS separat, SD la maximum 20 MHz, LCD la 40 MHz, alimentare SD la 3.3 V si CS-urile inactive inainte de init.
2. Stabileste contractul fisierelor: nume si ID-uri, format WAV PCM mono 16 kHz/16-bit, regulile de validare si CRC-ul de referinta. Confirma cele trei sunete cu fallback fix in flash.
3. Refactorizeaza executia pe nuclee reale: Core 0 pentru evenimente/audio/W5500 si Core 1 pentru SD/LCD. Defineste protocolul de evenimente si sincronizarea cache-ului audio intre nuclee.
4. Initializeaza si valideaza SPI0 partajat si accesul la SD/LCD exclusiv din Core 1; masoara indexarea/incarcarea SD in timp ce LCD-ul este folosit.
5. Implementeaza catalogul de sunete: scanare la boot, validare RIFF/WAVE si CRC, incarcare integrala in cache a fisierelor selectate si fallback-urile din flash pentru cele trei sunete principale.
6. Masoara dimensiunea totala a sunetelor si RAM-ul utilizabil; decide daca se incarca toate sunetele sau doar cele trei principale si daca se foloseste cache RAM sau flash.
7. Implementeaza motorul audio I2S PIO + DMA la 16 kHz, cu buffer DMA dimensionat separat. Core 0 reda din cache-ul validat; redarea nu citeste direct de pe SD.
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
- Comportamentul la evenimente audio simultane: intrerupere, coada sau prioritati.
- Masurarea latentei pornirii sub 1 ms din cache. Masoara separat indexarea/reincarcarea SD si efectul acestora asupra actualizarii LCD; redarea nu citeste de pe card.
- Implementarea separarii reale pe nuclee (de exemplu `setup1()`/`loop1()` pe Core 1) si a sincronizarii cache-ului audio intre nuclee; in codul curent `runCore0()` si `runCore1()` sunt apelate secvential din aceeasi bucla.
- Fluxul web pentru incarcare si stergere, inclusiv protectia operatiilor cat timp un fisier este folosit la redare.
- Accesul direct de pe PC si procedura recomandata pentru actualizarea fisierelor pe card.
- Stabilirea modului de salvare/persistenta a maparii eveniment-sunet configurate din web.
- Stabilirea ID-urilor si argumentelor exacte pentru comenzile distincte de redare din service box (`mb_xx`) si din interfata web.
- Extinderea maparii eveniment-audio pentru evenimentele viitoare.
