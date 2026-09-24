# Rezultate

## Partea matematică

Artefactele curente raportează toate verificările numerice ca `PASS`. Valorile provin din `informatica/artifacts/bridge_results.json` și `informatica/artifacts/diophantine_results.json`.

| verificare | rezultat observat | valoare de referință |
| --- | ---: | ---: |
| Asimptotic constant, `sqrt(2)` | `0.2513000154` | `0.25` |
| Asymptotic constant, `phi` | `0.3665687179` | `0.3618033989` |
| Exponent transfer | `mu_geo = 1.9982985127`, `R^2 = 0.9799496319` | `mu = 2` |
| Steinhaus-Sos, `N=1000,5000,20000` | 3 gap-uri la fiecare scară | `<= 3` |
| Closure dictionary, `k=3` | 321 vs. 321, diferență simetrică 0 | 321, 0 |
| Diophantine module 1 | `PASS` | test implementat |
| Diophantine module 2 | slope `1.0`, `R^2=1.0` | slope `1` |
| Diophantine module 3 | aceleași 3 gap-uri și varianțe ca bridge | verificare internă |
| Diophantine module 4 | 25,346 păstrate, 24,654 eliminate, 25,186 pozitive | test implementat |
| Diophantine module 5 | `k3=321`, `k4_engineered=71`, `k4_unrelated=9` | test implementat |

Aceste rezultate validează experimentele matematice existente. Ele nu demonstrează extensia teoretică completă la aproximarea simultană pentru `k > 3`.

## Partea informatică

Datasetul executabil are 160 de semnături, `ell = 12`, traces sintetice și split fix de 96 train, 24 validation și 40 test/attack. CNN-ul și HNP primesc aceleași 40 de ID-uri pentru etapa de atac.

Rularea curentă, refăcută după trecerea CNN-ului la 100 de epoci, a produs:

| metrică | rezultat |
| --- | ---: |
| validation bit accuracy | `99.65%` |
| validation prefix accuracy | `95.83%` |
| test bit accuracy | `100.00%` |
| test prefix accuracy | `100.00%` (`40/40`) |
| HNP oracle, `ell=12, m=40` | cheie recuperată |
| CNN -> HNP, `ell=12, m=40` | cheie recuperată |
| backend local | `sympy` |
| `fpylll` disponibil local | `false` |

Rezultatul HNP cu SymPy demonstrează că lanțul CNN -> HNP funcționează pe instanța sintetică de bază. Proiectul nu etichetează această rulare drept `PASS` de referință, deoarece backendul cerut pentru execuția de referință este `fpylll`. În CI, `PASS` cere `fpylll` și verificarea candidatului pe cheia generată în același job.

Tabelul de bază este în `results/tables/cnn-hnp-baseline.csv`.

## Analiza în `(ell, sigma, m)`

Codul pentru analiza cantitativă este `informatica/experiments/benchmark_cnn_hnp_surface.py`. Pentru fiecare combinație măsoară separat:

- acuratețea CNN pe bit și pe prefix;
- recuperarea HNP cu prefixe oracle;
- recuperarea HNP cu prefixe produse de CNN;
- backendul LLL și timpul de rulare.

Configurația de referință folosește `ell ∈ {8,12}`, `sigma ∈ {0.10,0.15,0.20,0.25}` și `m ∈ {20,40}`. Workflow-ul `.github/workflows/benchmark.yml` rulează sweep-ul cu `fpylll` și încarcă CSV-ul ca artefact.

Matricea completă nu este declarată rezultată local până când workflow-ul cu `fpylll` nu rulează. Mediul curent nu poate instala `fpylll` din PyPI.
