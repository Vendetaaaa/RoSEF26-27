# Prezentarea proiectului

Proiectul leagă două direcții care se întâlnesc în aceeași problemă: partea informatică construiește un experiment controlat pentru ECDSA, iar partea matematică studiază concurența aritmetică și aproximarea diofantică.

## Lanțul informatic

`dataset_generator3.py` generează chei, nonce-uri și semnături ECDSA pe secp256k1. Datasetul public conține numai date disponibile atacului. Oracle-ul păstrează cheia privată și nonce-urile complete pentru verificarea experimentală.

`ecdsa_leakage_model4.py` transformă nonce-urile într-un model sintetic bazat pe Hamming Weight / Hamming Distance și adaugă zgomot gaussian. Configurația de bază folosește `sigma = 0.15`, 256 de eșantioane pe trace și primii 12 biți MSB ai nonce-ului ca țintă pentru CNN.

`cnn/train.py` și `cnn_nonce_analysis5.py` folosesc aceeași împărțire explicită: 96 de eșantioane pentru train, 24 pentru validation și 40 pentru test/attack. CNN-ul raportează rezultatele pe validation și produce exact cele 40 de prefixe folosite de etapa HNP.

`hnp_attack1.py` construiește termenii HNP și rulează LLL prin backendul selectat. `fpylll` este backendul de referință, iar `--require-fpylll` refuză fallback-ul SymPy. În dezvoltare, SymPy poate confirma lanțul CNN → HNP, dar rezultatul este marcat `PASS_DEVELOPMENT` și nu este prezentat drept execuție de referință.

## Starea experimentală

Validarea matematică existentă este împărțită în două grupuri de artefacte. `bridge_results.json` conține patru verificări numerice, iar `diophantine_results.json` conține încă cinci. Aceste rezultate documentează separat verificările matematice și nu sunt confundate cu recuperarea cheii prin atacul HNP.

Instanța oracle cu 40 de semnături poate fi redusă prin LLL-ul SymPy și returnează cheia generată pentru experiment. Această rulare verifică lanțul algebric și solverul în mediul de dezvoltare, dar nu demonstrează execuția backendului `fpylll`.

Execuția de referință trebuie să folosească `fpylll` și să treacă verificarea completă CNN → HNP. În acest caz, rezultatul poate fi raportat ca `PASS`.

## Experimente cantitative

`informatica/experiments/benchmark_cnn_hnp_surface.py` construiește matricea de experimente pentru `ell`, `sigma` și `m`. Workflow-ul `.github/workflows/benchmark.yml` repetă configurațiile pe trei seed-uri și păstrează separat rezultatele CNN, recuperarea oracle și recuperarea bazată pe prefixele CNN.

Un singur experiment de bază arată comportamentul unei configurații; rata de succes pentru o matrice de parametri trebuie calculată din toate rulările prevăzute de benchmark.

## Limita matematică

Partea matematică riguroasă acoperă cazul scalar cu trei familii și transferul exponentului asociat. Codul conține experimente pentru familii suplimentare, dar acestea nu trebuie prezentate ca o demonstrație riguroasă a cazului general de aproximare simultană pentru `k > 3`.

## Etapa hardware

Firmware-ul și protocolul ESP32 sunt păstrate în repository, dar etapa hardware nu face parte din rezultatele curente. Datasetul folosit în pipeline este sintetic, iar măsurători fizice nu sunt revendicate până când nu există traces și metadata verificabile.
