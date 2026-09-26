# Prezentarea proiectului

Proiectul leagă două direcții care se întâlnesc în aceeași problemă: partea informatică construiește un experiment controlat pentru ECDSA, iar partea matematică studiază concurența aritmetică și aproximarea diofantică.

## Lanțul informatic

`dataset_generator3.py` generează chei, nonce-uri și semnături ECDSA pe secp256k1. Datasetul public conține numai date disponibile atacului. Oracle-ul păstrează cheia privată și nonce-urile complete pentru verificarea experimentală.

`ecdsa_leakage_model4.py` transformă nonce-urile într-un model sintetic bazat pe Hamming Weight / Hamming Distance și adaugă zgomot gaussian. Configurația de bază folosește `sigma = 0.15`, 256 de eșantioane pe trace și primii 12 biți MSB ai nonce-ului ca țintă pentru CNN.

`cnn/train.py` și `cnn_nonce_analysis5.py` folosesc aceeași împărțire explicită: 96 de eșantioane pentru train, 24 pentru validation și 40 pentru test/attack. CNN-ul raportează rezultatele pe validation și produce exact cele 40 de prefixe folosite de etapa HNP.

`hnp_attack1.py` construiește termenii HNP și rulează LLL prin backendul selectat. `fpylll` este backendul de referință, iar `--require-fpylll` refuză fallback-ul SymPy. În dezvoltare, SymPy poate confirma lanțul CNN → HNP, dar rezultatul este marcat `PASS_DEVELOPMENT` și nu este prezentat drept execuție de referință.

## Starea experimentală

Validarea matematică existentă este împărțită în două grupuri de artefacte. `bridge_results.json` conține patru verificări numerice, iar `diophantine_results.json` conține încă cinci. Aceste rezultate documentează separat verificările matematice și nu sunt confundate cu recuperarea cheii prin atacul HNP.

Execuția de referință cu `fpylll` a verificat lanțul CNN → HNP pe instanța sintetică de bază. Pentru benchmark, oracle-ul folosește prefixele corecte, iar CNN → HNP folosește prefixele produse de model. Rezultatele sunt raportate separat.

## Experimente cantitative

`informatica/experiments/benchmark_cnn_hnp_surface.py` construiește matricea de experimente pentru `ell`, `sigma` și `m`. Sweep-ul folosește:

- `ell ∈ {8, 12}`;
- `sigma ∈ {0.10, 0.15, 0.20, 0.25}`;
- `m ∈ {20, 40}`;
- seed-urile `20260919`, `20260920`, `20260921`;
- 100 de epoci.

Fișierele rezultate sunt `results/tables/cnn-hnp-parameter-sweep.csv` și `results/tables/cnn-hnp-parameter-sweep-summary.csv`.

Pentru `m = 20`, benchmarkul folosește primele 20 de ID-uri din același set de test de 40 de ID-uri folosit pentru `m = 40`.

## Limita matematică

Partea matematică riguroasă acoperă cazul scalar cu trei familii și transferul exponentului asociat. Codul conține experimente pentru familii suplimentare, dar acestea nu trebuie prezentate ca o demonstrație riguroasă a cazului general de aproximare simultană pentru `k > 3`.

## Etapa hardware

Firmware-ul și protocolul ESP32 sunt păstrate în repository, dar etapa hardware nu face parte din rezultatele curente. Datasetul folosit în pipeline este sintetic, iar măsurători fizice nu sunt revendicate până când nu există traces și metadata verificabile.
