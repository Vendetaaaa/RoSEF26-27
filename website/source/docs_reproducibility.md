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

În arhiva publică, `informatica/artifacts/oracle_dataset.json` păstrează nonce-urile de test, dar cheia privată de ground truth este redactată. De aceea, Notebook 04 raportează separat `PASS` pentru rularea de referință înregistrată și `NOT EXECUTED` pentru reluarea locală atunci când cheia de ground truth sau `fpylll` nu este disponibilă. Pentru o reluare completă, rulează `python informatica/ECDSA/main.py --require-fpylll` într-un mediu local controlat. Nu publica fișierul oracle rezultat din această rulare.

## Date și split

Experimentul folosește 160 de semnături și păstrează aceeași împărțire în toate etapele:

- 96 train;
- 24 validation;
- 40 test/attack.

Fișierul `informatica/artifacts/dataset_split.json` este sursa explicită pentru această împărțire. Etapa HNP folosește exact cele 40 de exemple din test.

## Benchmarkul cantitativ

Sweep-ul regenerat folosește 100 de seed-uri, de la `1000` la `1099`, 100 de epoci, `ell` în `{8, 10, 12, 14}`, `sigma` în `{0, 0.05, 0.10, 0.15, 0.20}` și `m = 40`.

Pentru a reproduce configurațiile salvate, rulează din rădăcina repository-ului:

```bash
python informatica/experiments/benchmark_cnn_hnp_surface.py \
  --leaked-bits 8 10 12 14 \
  --sigmas 0 0.05 0.10 0.15 0.20 \
  --sample-counts 40 \
  --seeds $(seq 1000 1099) \
  --epochs 100 \
  --output results/tables/cnn-hnp-parameter-sweep.csv
```

Execuția de referință cere `fpylll`. Comanda scrie `results/tables/cnn-hnp-parameter-sweep.csv` și rezumatul agregat `results/tables/cnn-hnp-parameter-sweep-summary.csv`.

`results/tables/hnp-oracle-thresholds.csv` păstrează verificarea oracle pentru 400 de combinații. Valorile cheilor private recuperate sunt redactate.

## CI

`.github/workflows/tests.yml` verifică importul `fpylll`, rulează pipeline-ul complet cu `--require-fpylll` și cere `hnp = PASS`, `hnp_backend = fpylll`, `fully_executed = true` și recuperarea validată în lanțul CNN către HNP.

`.github/workflows/reproducibility.yml` repetă verificarea de referință. `.github/workflows/benchmark.yml` rulează matricea cantitativă și păstrează rezultatele benchmarkului ca artefact GitHub Actions.

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
