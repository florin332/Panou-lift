# Sound TODO

## Decizii stabilite

- Sunetele nu vor mai fi hardcodate/incluse in firmware; vor fi citite de pe cardul SD onboard al Marble Pico.
- Format audio: WAV cu PCM, mono, 16 kHz; iesirea I2S va ramane la 16 kHz, aceeasi rata ca fisierele, pentru a evita resampling-ul daca decoderul permite.
- Se pastreaza momentan cele trei evenimente audio existente: alarma, suprasarcina si oprire. Denumirile fisierelor vor ramane aceleasi ca cele folosite pentru sunetele curente; continutul audio de pe card poate fi diferit de cel incorporat acum in flash.
- Cardul SD este instalat intern si nu este accesibil din exterior; lipsa cardului nu este un caz normal care necesita tratare dedicata.
- Se doreste pastrarea in flash a unei copii de rezerva pentru cele 3-4 sunete principale. La declansarea unui eveniment, redarea incearca fisierul de pe SD; daca fisierul nu poate fi citit sau nu trece verificarea integritatii, se reda copia corespunzatoare din flash.
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

W5500 ramane pe SPI1 (GP12-GP15), separat de cardul SD si LCD.

## Starea actuala

- Driverul `Sound` reda in prezent WAV-uri incorporate in flash prin `AudioFileSourcePROGMEM` si `AudioGeneratorWAV`.
- Iesirea audio este I2S catre MAX98357A, pe GP8 (BCLK), GP9 (LRCLK) si GP10 (DATA).
- Rata I2S curenta este 16 kHz, iar gain-ul este gestionat prin setarile persistente existente. Se pastreaza iesirea I2S la 16 kHz pentru fisierele WAV de 16 kHz.
- Interfata actuala expune `alarm()`, `overload()` si `liftStopped()`; acestea trebuie sa ramana asociate celor trei evenimente de mai sus.
- Reper build curent, cu cele trei sunete si o imagine in flash: 557752 din 8384512 bytes (6.7%). Aceasta este utilizarea totala raportata pentru firmware, nu dimensiunea individuala a resurselor audio sau imaginii.

## De stabilit / verificat la implementare

- Numele exacte ale fisierelor, extensiile si locatia lor pe card.
- Validarea formatului WAV acceptat, inclusiv adancimea de biti si codec-ul PCM; confirmarea ca decoderul reda la 16 kHz fara conversie de rata.
- Comportamentul pentru fisier lipsa, corupt sau cu eroare de citire: fallback la copia din flash pentru sunetele principale; stabilirea comportamentului pentru sunetele fara copie in flash.
- Stabilirea verificarii integritatii fisierelor audio cu CRC. CRC-ul calculat trebuie comparat cu o valoare de referinta de incredere; calcularea CRC-ului fara valoare asteptata nu detecteaza coruperea.
- Stabilirea momentului verificarii CRC (la incarcare, la initializare sau inainte de redare) si a locului unde se pastreaza valoarea CRC; incarcarile si stergerile din web trebuie sa mentina aceste date sincronizate.
- Selectarea celor 3-4 sunete care vor avea copie de rezerva in flash.
- Comportamentul la evenimente audio simultane: intrerupere, coada sau prioritati.
- Verificarea latentei citirilor SD si a partajarii SPI cu LCD-ul; stabilirea bufferelor si a modului de acces fara blocarea buclei principale. Fallback-ul pentru erori nu previne intreruperile cauzate doar de citiri lente.
- Fluxul web pentru incarcare si stergere, inclusiv protectia operatiilor cat timp un fisier este folosit la redare.
- Accesul direct de pe PC si procedura recomandata pentru actualizarea fisierelor pe card.
- Stabilirea modului de salvare/persistenta a maparii eveniment-sunet configurate din web.
- Stabilirea ID-urilor si argumentelor exacte pentru comenzile distincte de redare din service box (`mb_xx`) si din interfata web.
- Extinderea maparii eveniment-audio pentru evenimentele viitoare.
