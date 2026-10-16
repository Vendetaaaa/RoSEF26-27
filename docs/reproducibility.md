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

În arhiva publică, `informatica/artifacts/oracle_dataset.json` păstrează nonce-urile complete de test, iar câmpul explicit al cheii private este redactat. Nonce-urile complete împreună cu semnăturile permit derivarea cheii sintetice, deci acest fișier este ground truth pentru evaluare, nu date de intrare pentru un atac orb. Codul de atac trebuie să consume datasetul public și prefixele prezise; cheia sintetică se folosește numai după execuția solverului, la evaluare. Notebook 04 raportează separat starea rulării de referință înregistrate și `NOT EXECUTED` pentru reluarea locală atunci când datele necesare nu sunt disponibile. Pentru o reluare completă, rulează `python informatica/ECDSA/main.py --require-fpylll` într-un mediu local controlat și nu publica fișierele oracle generate local cu secrete explicite.

## Date și split

Experimentul folosește 160 de semnături și păstrează aceeași împărțire în toate etapele:

- 96 train;
- 24 validation;
- 40 test/attack.

Fișierul `informatica/artifacts/dataset_split.json` este sursa explicită pentru această împărțire. Etapa HNP folosește exact cele 40 de exemple din test.

## Benchmarkul cantitativ

Sweep-ul înregistrat folosește 100 de seed-uri, de la `1000` la `1099`, 100 de epoci, `ell` în `{8, 10, 12, 14}`, `sigma` în `{0, 0.05, 0.10, 0.15, 0.20}` și `m = 40`. CSV-urile existente sunt rezultate istorice: versiunea anterioară a orchestratorului făcea un test de compatibilitate cu cheia de ground truth înainte de LLL, iar timpii pentru acele instanțe nu sunt end-to-end. Codul curent nu mai folosește cheia secretă pentru a decide dacă rulează solverul. Înainte de a cita timpi pentru întregul sweep, regenerează CSV-urile cu versiunea curentă a scriptului.

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

`.github/workflows/reproducibility.yml` repetă verificarea de referință. `.github/workflows/benchmark.yml` rulează un sweep exploratoriu scurt, cu trei seed-uri, și păstrează rezultatele ca artefact GitHub Actions; nu înlocuiește reproducerea completă pe 100 de seed-uri descrisă mai sus.

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

## Reproducerea experimentelor temporale suplimentare

Experimentele temporale folosesc 160 de exemple ECDSA,
cu un split fix: 96 train, 24 validation și 40 test.

Cele 100 de seed-uri (1400–1499) controlează zgomotul
și inițializările CNN. Datasetul și cheia privată
rămân aceleași. Aceste repetări nu reprezintă
100 de chei private independente.

### Scripturi experimentale

- `informatica/experiments/temporal_leakage_model.py`
- `informatica/experiments/temporal_viterbi.py`
- `informatica/experiments/benchmark_temporal_100.py`
- `informatica/experiments/benchmark_cnn_confidence_100.py`
- `informatica/experiments/benchmark_hnp_error_sensitivity.py`

### Fișiere cu rezultate

- `results/validation/temporal-leakage/temporal-multiseed-100.csv`
- `results/validation/cnn-confidence-hnp/confidence-selection-100.csv`
- `results/validation/hnp-error-sensitivity/hnp-error-sensitivity.csv`

### Precizări metodologice

Decodorul Viterbi cunoaște coeficienții modelului
temporal și reprezintă un baseline informat.

Selecția după încrederea CNN îmbunătățește
acuratețea prefixelor, dar nu garantează
recuperarea cheii private.

Experimentul de sensibilitate HNP folosește
prefixe oracle cu erori introduse controlat.
Rezultatele descriu configurațiile testate,
nu o limită teoretică universală.

Reproducerea integrală necesită verificarea
dependențelor, a datasetului de intrare și
a disponibilității datelor oracle necesare
experimentelor. Nu publicați valori private
sau nonce-uri secrete fără verificare.

### Verificarea testelor

Comandă: PYTHONDONTWRITEBYTECODE=1 python -m pytest -q -p no:cacheprovider

Verificarea modificărilor: git diff --check

## Regenerarea deterministă a datasetului oracle

Datasetul ECDSA utilizat în experimente poate fi regenerat
determinist folosind `informatica/ECDSA/dataset_generator3.py`.

Parametrii datasetului de referință sunt:

- Curba: secp256k1.
- Număr de semnături: 160.
- Biți MSB cunoscuți: 12.
- Seed generator: `20260919`.

Verificarea efectuată a confirmat că datasetul public regenerat
este identic cu cel existent și că toate cele 160 de nonce-uri
regenerate coincid cu oracle-ul experimental original.

Datasetul oracle public are nonce-urile redactate. Pentru
rularea benchmarkurilor care necesită ground truth, se poate
genera local un oracle numeric și se poate indica fișierul
prin variabila de mediu `ROSEF_ORACLE_DATASET`.

Nu se recomandă rularea directă a pipeline-ului principal
pentru această operație, deoarece acesta poate suprascrie
artefactele existente în `informatica/artifacts/`.

Generatorul utilizează un seed determinist, nu un generator
criptografic de numere aleatoare. Cheia privată și nonce-urile
sunt date sintetice reproductibile, nu secrete criptografice
reale. Reproducerea exactă a datasetului a fost verificată;
reluarea integrală a tuturor benchmarkurilor după modificările
de cod nu a fost încă verificată.
