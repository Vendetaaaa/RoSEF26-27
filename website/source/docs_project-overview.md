# Prezentarea proiectului

Proiectul leagă două direcții care se întâlnesc în aceeași problemă: partea informatică construiește un experiment controlat pentru ECDSA, iar partea matematică studiază concurența aritmetică și aproximarea diofantică.

## Lanțul informatic

`dataset_generator3.py` generează chei, nonce-uri și semnături ECDSA pe secp256k1. Datasetul public conține numai date disponibile atacului. În mediul local controlat, oracle-ul păstrează cheia privată și nonce-urile complete pentru verificarea experimentală; copia publică redactează cheia privată.

`ecdsa_leakage_model4.py` transformă nonce-urile într-un model sintetic bazat pe Hamming Weight / Hamming Distance și adaugă zgomot gaussian. Configurația de bază folosește `sigma = 0.15`, 256 de eșantioane pe trace și primii 12 biți MSB ai nonce-ului ca țintă pentru CNN.

`cnn/train.py` și `cnn_nonce_analysis5.py` folosesc aceeași împărțire explicită: 96 de eșantioane pentru train, 24 pentru validation și 40 pentru test/attack. CNN-ul raportează rezultatele pe validation și produce exact cele 40 de prefixe folosite de etapa HNP.

`hnp_attack1.py` construiește termenii HNP și rulează LLL prin backendul selectat. `fpylll` este backendul de referință, iar `--require-fpylll` refuză fallback-ul SymPy. În dezvoltare, SymPy poate confirma lanțul CNN → HNP, dar rezultatul este marcat `PASS_DEVELOPMENT` și nu este prezentat drept execuție de referință.

## Starea experimentală

Validarea matematică existentă este împărțită în două grupuri de artefacte. `bridge_results.json` conține patru verificări numerice, iar `diophantine_results.json` conține încă cinci. Aceste rezultate documentează separat verificările matematice și nu sunt confundate cu recuperarea cheii prin atacul HNP.

Artefactele rulării de referință înregistrează `PASS` pentru lanțul CNN → HNP pe instanța sintetică de bază, cu backend `fpylll` și cheia verificată față de ground truth. Acest statut descrie acea instanță. Sweep-ul separat agregă 100 de seed-uri pentru fiecare dintre cele 20 de configurații; rata de recuperare CNN → HNP variază cu numărul de biți și nivelul zgomotului. Valorile sunt păstrate în `results/tables/cnn-hnp-parameter-sweep-summary.csv` și rezumate în `docs/project-summary.md`.

## Experimente cantitative

`informatica/experiments/benchmark_cnn_hnp_surface.py` evaluează acuratețea CNN și recuperarea HNP pentru 20 de configurații. Sweep-ul disponibil folosește 100 de seed-uri, de la `1000` la `1099`, 100 de epoci, `ell` în `{8, 10, 12, 14}`, `sigma` în `{0, 0.05, 0.10, 0.15, 0.20}` și `m = 40`.

Tabelul detaliat este `results/tables/cnn-hnp-parameter-sweep.csv`, rezumatul agregat este `results/tables/cnn-hnp-parameter-sweep-summary.csv`, iar pragurile oracle sunt în `results/tables/hnp-oracle-thresholds.csv`.

## Limita matematică

Partea matematică riguroasă acoperă cazul scalar cu trei familii și transferul exponentului asociat. Codul conține experimente pentru familii suplimentare, dar acestea nu trebuie prezentate ca o demonstrație riguroasă a cazului general de aproximare simultană pentru `k > 3`.

## Etapa hardware

Firmware-ul și protocolul ESP32 sunt păstrate în repository, dar etapa hardware nu face parte din rezultatele curente.
