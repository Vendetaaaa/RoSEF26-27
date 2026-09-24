# Reproductibilitate

Reproductibilitatea proiectului se bazează pe aceleași date, aceleași splituri, aceleași configurații și același backend LLL atunci când se execută experimentul de referință.

## Mediul recomandat

Dependențele proiectului sunt declarate în `requirements.txt`. Lista include `fpylll`; pentru importul complet al acestei biblioteci, mediul trebuie să aibă și `cysignals` disponibil.

Instalarea de bază este:

```bash
python -m pip install -r requirements.txt
```

Apoi pot fi rulate testele:

```bash
python -m pytest -v
```

Pentru dezvoltare, `main.py` poate folosi SymPy atunci când `fpylll` nu este disponibil. Un astfel de rezultat este etichetat `PASS_DEVELOPMENT`.

Execuția de referință trebuie să refuze fallbackul:

```bash
python informatica/ECDSA/main.py --require-fpylll
```

Această comandă trebuie să ruleze cu backendul `fpylll` și să verifice lanțul CNN → HNP.

## Backendul LLL

`HNPLatticeSolver.solve()` returnează candidatul împreună cu backendul folosit. Opțiunea `require_fpylll=True` oprește execuția atunci când `fpylll` nu este disponibil, în loc să schimbe automat backendul.

`hnp_attack1.py` scrie în `hnp_result.json` backendul efectiv, politica de backend și starea validării. Astfel, o verificare algebrică realizată cu SymPy nu poate fi prezentată accidental drept execuție de referință cu `fpylll`.

## Date și split

Experimentul folosește 160 de semnături și păstrează aceeași împărțire în toate etapele:

- 96 train;
- 24 validation;
- 40 test/attack.

Fișierul `informatica/artifacts/dataset_split.json` este sursa explicită pentru această împărțire. Etapa HNP folosește exact cele 40 de exemple din test.

## CI

`.github/workflows/tests.yml` verifică importul `fpylll`, rulează pipeline-ul complet cu `--require-fpylll` și cere:

- `hnp = PASS`;
- `hnp_backend = fpylll`;
- `fully_executed = true`;
- recuperare validată în lanțul CNN → HNP.

`.github/workflows/reproducibility.yml` repetă verificarea de referință.

`.github/workflows/benchmark.yml` rulează matricea `(ell, sigma, m)` și păstrează rezultatele benchmarkului ca artefact GitHub Actions.

Dacă importul `fpylll` eșuează din cauza unei dependențe precum `cysignals`, CI nu trebuie considerat o verificare trecută. Dependența trebuie instalată înainte ca testul de import și recuperarea oracle să ruleze.

## Verificarea locală

Pentru o verificare de dezvoltare se poate rula:

```bash
python informatica/ECDSA/main.py
```

Dacă mediul nu are `fpylll`, rezultatul poate folosi SymPy și trebuie interpretat ca `PASS_DEVELOPMENT`.

Pentru verificarea cerută la predare, trebuie folosit:

```bash
python informatica/ECDSA/main.py --require-fpylll
```

și trebuie verificat rezultatul produs în `informatica/artifacts/hnp_result.json`.

## Hardware

Etapa hardware rămâne în afara pipeline-ului executat. Datasetul actual este sintetic, iar documentația nu prezintă măsurători reale ca rezultate. Firmware-ul și protocolul ESP32 rămân în repository pentru etapa experimentală ulterioară.
