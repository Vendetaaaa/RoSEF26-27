# Rezultate

## Partea matematică

Artefactele curente raportează verificările numerice ca `PASS`. Valorile provin din `informatica/artifacts/bridge_results.json` și `informatica/artifacts/diophantine_results.json`.

| verificare | rezultat observat | valoare de referință |
| --- | ---: | ---: |
| Asymptotic constant, `sqrt(2)` | `0.2513000154` | `0.25` |
| Asymptotic constant, `phi` | `0.3665687179` | `0.3618033989` |
| Exponent transfer | `mu_geo = 1.9982985127`, `R^2 = 0.9799496319` | `mu = 2` |
| Steinhaus-Sos, `N=1000,5000,20000` | 3 gap-uri la fiecare scară | `<= 3` |
| Closure dictionary, `k=3` | 321 vs. 321, diferență simetrică 0 | 321, 0 |
| Diophantine module 1 | `PASS` | test implementat |
| Diophantine module 2 | slope `1.0`, `R^2=1.0` | slope `1` |
| Diophantine module 3 | aceleași 3 gap-uri și varianțe ca bridge | verificare internă |
| Diophantine module 4 | 25,346 păstrate, 24,654 eliminate, 25,186 pozitive | test implementat |
| Diophantine module 5 | `k3=321`, `k4_engineered=71`, `k4_unrelated=9` | test implementat |

Aceste rezultate validează experimentele numerice în domeniul testat. Ele nu reprezintă o demonstrație completă pentru aproximarea simultană în cazul general `k > 3`.

## Partea informatică

Rularea de referință a pipeline-ului folosește backendul `fpylll`. Pentru configurația de bază sunt raportate 160 de semnături sintetice, cu split fix de 96 train, 24 validation și 40 test/attack.

| metrică | rezultat |
| --- | ---: |
| validation bit accuracy | `99.65%` |
| validation prefix accuracy | `95.83%` |
| test bit accuracy | `100.00%` |
| test prefix accuracy | `100.00%` (`40/40`) |
| HNP oracle, `ell=12, m=40` | `PASS`, cheia de test recuperată și validată |
| CNN to HNP, `ell=12, m=40` | `PASS`, cheia de test recuperată și validată |
| backend de referință | `fpylll` |
| pipeline complet | `PASS` |

Rezultatele de referință sunt în `results/tables/final-results.csv`, iar starea detaliată este în `informatica/artifacts/pipeline_results.json` și `informatica/artifacts/hnp_result.json`. Cheia privată este redactată în artefactele publice.

## Sweep cantitativ

`results/tables/cnn-hnp-baseline.csv` păstrează tabelul baseline din arhiva existentă. Sweep-ul regenerat conține 2.000 de rulări în `results/tables/cnn-hnp-parameter-sweep.csv`, cu 100 de seed-uri pentru fiecare configurație din `ell = {8, 10, 12, 14}` și `sigma = {0, 0.05, 0.10, 0.15, 0.20}`. Fiecare rulare folosește 100 de epoci. Rezumatul agregat cu 20 de configurații este în `results/tables/cnn-hnp-parameter-sweep-summary.csv`.

Tabelul `results/tables/hnp-oracle-thresholds.csv` conține rezultatele oracle pentru praguri diferite. Valorile cheii recuperate sunt redactate.

`PASS` pentru pipeline și teste confirmă că execuția a funcționat conform criteriului definit. Rata de recuperare poate fi mai mică pentru anumite configurații, de exemplu când numărul de biți scurși este redus sau zgomotul este mare. Aceste valori rămân rezultate experimentale și nu sunt schimbate artificial în succese.

## Hardware

Rezultatele raportate aici provin din dataseturi sintetice. Etapa ESP32 este păstrată în proiect, dar acest raport nu revendică măsurători fizice care nu sunt incluse în artefactele curente.
