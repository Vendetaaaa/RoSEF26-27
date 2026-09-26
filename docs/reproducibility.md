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

## Benchmarkul cantitativ

Sweep-ul de referință se poate reproduce cu:

```bash
python informatica/experiments/benchmark_cnn_hnp_surface.py   --leaked-bits 8 12   --sigmas 0.10 0.15 0.20 0.25   --sample-counts 20 40   --seeds 20260919 20260920 20260921   --epochs 100
```

Execuția de referință cere `fpylll`. Benchmarkul scrie:

```text
results/tables/cnn-hnp-parameter-sweep.csv
results/tables/cnn-hnp-parameter-sweep-summary.csv
```

Pentru `m = 20`, benchmarkul folosește primele 20 de ID-uri din setul de test de 40 de ID-uri. Prin urmare, rezultatele pentru `m = 20` și `m = 40` provin din același split și au o parte comună de date.

## CI

`.github/workflows/tests.yml` verifică importul `fpylll`, rulează pipeline-ul complet cu `--require-fpylll` și cere:

- `hnp = PASS`;
- `hnp_backend = fpylll`;
- `fully_executed = true`;
- recuperare validată în lanțul CNN → HNP.

`.github/workflows/reproducibility.yml` repetă verificarea de referință.

`.github/workflows/becnhmark.yaml` rulează matricea `(ell, sigma, m)` și păstrează rezultatele benchmarkului ca artefact GitHub Actions.

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

Etapa hardware rămâne în afara pipeline-ului executat.
