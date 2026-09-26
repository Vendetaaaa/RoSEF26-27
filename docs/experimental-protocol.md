# Protocol experimental

## Configurația de bază

Datasetul de bază conține 160 de semnături ECDSA pe secp256k1. Generatorul folosește seed-ul `20260919`, fixează `ell = 12` biți MSB ai nonce-ului și construiește traces sintetice cu 256 de poziții. Profilul CNN folosește modelul bazat pe Hamming Weight și zgomot gaussian cu `sigma = 0.15`.

CNN-ul rulează 100 de epoci cu learning rate `0.005`. Modelul păstrează starea cu cea mai bună acuratețe pe validation.

## Split

Fișierul `informatica/artifacts/dataset_split.json` este sursa unică pentru împărțirea datelor.

| split | eșantioane | rol |
| --- | ---: | --- |
| train | 96 | antrenarea CNN |
| validation | 24 | evaluarea și selecția modelului |
| test | 40 | evaluarea finală și intrarea în HNP |

Nu se generează un al doilea split implicit. CNN-ul produce exact 40 de predicții pentru setul folosit de HNP.

## Ordinea experimentului

1. Se generează cele 160 de semnături și se validează relațiile ECDSA.
2. Se generează traces sintetice pentru aceleași ID-uri.
3. CNN-ul este antrenat numai pe setul `train`.
4. Se măsoară rezultatele pe validation și test, apoi se scriu prefixele pentru cele 40 de eșantioane test.
5. Se construiește instanța HNP din datele publice și prefixele produse de CNN.
6. Se rulează LLL cu backendul `fpylll` în execuția de referință și se validează candidatul pe relațiile HNP.
7. Se rulează separat instanța oracle pentru a măsura capacitatea lattice-ului fără eroarea CNN.

## Criteriul de succes

O recuperare HNP primește `PASS` numai când solverul returnează cheia corectă, candidatul trece limitele HNP și execuția de referință folosește backendul `fpylll`.

Când aceeași verificare este realizată cu SymPy în mediul de dezvoltare, rezultatul este etichetat `PASS_DEVELOPMENT`. Această etichetă păstrează diferența dintre validarea locală și execuția de referință.

## Matricea cantitativă `(ell, sigma, m)`

`informatica/experiments/benchmark_cnn_hnp_surface.py` rulează combinațiile:
`ell ∈ {8, 12}`, `sigma ∈ {0.10, 0.15, 0.20, 0.25}` și
`m ∈ {20, 40}`. Benchmarkul folosește 100 de epoci și de la `1000`-`1099` seed-uri.

Pentru fiecare configurație se păstrează acuratețea CNN pe bit și pe prefix,
rezultatul HNP oracle, rezultatul HNP din prefixele CNN, backendul și timpii
de execuție. Pentru `m = 20` se folosesc primele 20 de ID-uri din același
set de 40 de teste folosit pentru `m = 40`; cele două valori nu reprezintă
două seturi independente.

Fișierele rezultate sunt:
- `results/tables/cnn-hnp-parameter-sweep.csv`;
- `results/tables/cnn-hnp-parameter-sweep-summary.csv`.

## Hardware

Etapa hardware rămâne neexecutată până când există traces fizice și metadata verificabile. Niciun rezultat din partea sintetică nu este prezentat drept măsurătoare ESP32.
