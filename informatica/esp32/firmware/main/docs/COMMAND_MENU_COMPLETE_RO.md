# Catalog complet al comenzilor ROSEF ESP32

Acest catalog a fost comparat cu dispatch-ul din `src/protocol/protocol.cpp` (`handleLine`). Meniul serial este emis la pornire și poate fi reafișat cu `HELP`, `MENU` sau `?`.

## Diagnostice și stare

| Comandă | Efect |
|---|---|
| `HELP` | Afișează meniul complet |
| `MENU` | Alias pentru `HELP` |
| `?` | Alias pentru `HELP` |
| `PING` | Verifică legătura serială; așteaptă `OK PONG` |
| `INFO` | Afișează pinii, baud rate-ul, INA219, OLED și LED-ul RGB |
| `STATUS` | Afișează uptime, heap disponibil, frecvența CPU, nivelul GPIO4 și starea butonului |
| `FINAL` | Captură unică de test: exact o semnare ECDSA, trigger GPIO4 activ-LOW și citiri INA219 cu timestamp înainte/în timpul/după semnare; fără batch și fără exportarea cheii private |
| `SELFTEST` | Rulează autotestele criptografice cu triggerul extern suprimat |
| `TESTPR` | Test secvențial pentru GPIO-urile configurate; apoi restaurează GPIO4 la HIGH și GPIO27 ca intrare pentru buton |
| `BUTTON` | Citește starea butonului de pe GPIO27 |
| `BUTTON_EVENT` | Citește și consumă evenimentul memorat de apăsare a butonului |
| `OLEDTEST` | Testează OLED-ul I²C 128×64 și afișează pinii/adresa detectată |

## INA219 și batch-uri

| Comandă | Argumente / limită | Efect |
|---|---|---|
| `INA219` | Fără argumente | O citire tensiune/curent/putere |
| `BATCH INA219 <count>` | `count`: 1–1000, obligatoriu | Citește repetat INA219 și emite câte o linie `SAMPLE` pentru fiecare citire |
| `BATCH <count>` | `count`: 1–1000, obligatoriu | Semnează/verifică repetat mesajul fix `rosef-batch`; nonce-ul de test este incrementat între operații; raportează read-back-ul GPIO4 |
| `SBATCH [count]` | Opțional; implicit 10, interval 1–1000 | Generator minim numai pentru GPIO4: fiecare ciclu ține LOW 250 ms și HIGH 250 ms. Nu rulează ECDSA sau INA219; folosește-l mai întâi ca să confirmi semnalul în PulseView |
| `BENCH <count>` | `count`: 1–1000, obligatoriu | Benchmark pentru durata operațiilor de semnare |

`BATCH INA219` fără valoarea `count` nu este o comandă completă și va raporta `ERR BAD_BATCH_COUNT`.

## Semnare ECDSA pentru teste

| Comandă | Format |
|---|---|
| `SIGN <message>` | Hash SHA-256 al mesajului textual, apoi semnare și verificare |
| `SIGNHEX <digest>` | `digest` este exact 64 de caractere hexazecimale (32 bytes) |
| `SIGNK <nonce> <message>` | `nonce` este scalar hex de 64 de caractere, urmat de un mesaj |
| `SIGNHEXK <nonce> <digest>` | Scalar nonce hex de 64 caractere, un spațiu, digest hex de 64 caractere |
| `SETK <nonce>` | Setează nonce-ul implicit din RAM; scalar secp256k1 valid, nenul, exact 64 caractere hex |
| `SETD <private-scalar>` | Setează cheia privată de test din RAM; scalar secp256k1 valid, nenul, exact 64 caractere hex |

Aceste comenzi sunt numai pentru cheile de test ale proiectului. `FINAL` refuză să ruleze dacă cheia activă diferă de vectorul de test compilat. Face exact o singură semnare cu nonce aleatoriu neafișat, colectează citiri INA219 în jurul ferestrei GPIO4 și raportează câte citiri se suprapun cu fereastra semnării. Nu exportă cheia privată și nu pretinde că recuperează cheia: INA219 furnizează citiri I2C cu rată redusă, nu o formă de undă analogică de mare viteză.

## Trigger GPIO4 și captură

| Comandă | Format / limită | Efect |
|---|---|---|
| `PULSE` | Fără argument: 1000 µs | Un impuls activ LOW pe GPIO4 |
| `PULSE <microseconds>` | 1–1.000.000 µs | Un impuls activ LOW pe GPIO4; raportează nivelul final citit de ESP32 |
| `TRIGTEST` | Fără argument: 8 cicluri | Impulsuri repetate, aproximativ 250 ms LOW și 250 ms HIGH per ciclu |
| `TRIGTEST <count>` | 1–12 cicluri | Număr specificat de impulsuri pentru testarea capturii externe |
| `FULLIMP` | Fără argumente | 160 de operații de semnare cu nonce-uri de test aleatorii și citiri INA219 cu rată joasă |
| `SCOPECAP` | Fără argumente | 160 de semnături cu metadate etichetate pentru captură externă; include etichete ground-truth numai pentru evaluarea datelor de test |

GPIO4 este HIGH în repaus și LOW în timpul impulsului. Comanda `SBATCH` produce impulsuri de diagnostic independente de semnarea ECDSA. `PASS` la read-back confirmă nivelul citit de ESP32, nu garantează că semnalul a ajuns la intrarea PulseView. `FULLIMP` folosește citiri I²C lente de la INA219 și nu produce o formă de undă de curent de mare viteză.


## OLED și LED RGB

OLED-ul I²C 128×64 folosește GPIO18 (SDA) și GPIO19 (SCK/SCL), separat de INA219 care rămâne pe GPIO21/22. Firmware-ul caută adresele `0x3C` și `0x3D`; `OLEDTEST` afișează configurația detectată. LED-ul RGB presupus este unul adresabil compatibil WS2812/NeoPixel, cu pinul de date pe GPIO25: verde = boot/comandă reușită, portocaliu = comandă în execuție, roșu = eroare.

## Aliasuri și comportamentul cu litere mici

Parserul transformă keyword-ul comenzii în majuscule, fără să transforme textul mesajului transmis către `SIGN`. `SBATCH [count]` generează impulsuri GPIO4 pentru diagnostic hardware; fiecare impuls este LOW 250 ms și apoi HIGH 250 ms. Nu face semnări criptografice și nu modifică implementarea existentă `BATCH <count>`. Pentru `BATCH INA219`, cuvântul `INA219` este normalizat separat. Sintaxa argumentelor este cea din tabele; folosește un spațiu simplu între keyword-uri și argumente.

## Limitări de verificare

Testele automate din `tests/test_source_contract.py` verifică prezența rutelor de comandă, acoperirea meniului și câteva contracte de regresie. Compilarea completă pentru placa ESP32 și fiecare comportament fizic trebuie validate cu Arduino toolchain și pe hardware-ul real.
