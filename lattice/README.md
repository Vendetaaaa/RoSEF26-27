# HNP lattice

Acest modul oferă interfața reutilizabilă pentru transformarea semnăturilor ECDSA și a prefixelor de nonce în termenii HNP folosiți de `HNPLatticeSolver`.

## API

`build_hnp_inputs(public_samples, leaked_prefixes, q, leaked_bits)` produce listele `t`, `u` și `a`. `HNPConfig` definește `B = 2^(nbits - leaked_bits)` și factorul de embedding. `HNPLatticeSolver` construiește baza, rulează LLL și validează candidatul.

`solve()` returnează perechea `(candidate, backend)`. `backend` este `fpylll` sau `sympy`. Apelul cu `require_fpylll=True` refuză fallback-ul SymPy și produce eroare dacă `fpylll` nu este instalat.

## Backend-uri

`fpylll` este backend-ul de referință. `sympy.Matrix.lll` rămâne fallback-ul pentru dezvoltare. Artefactele păstrează backend-ul efectiv și politica de execuție.

`PASS_DEVELOPMENT` înseamnă că LLL și recuperarea au funcționat cu SymPy pe instanța sintetică. `PASS` cere `fpylll` și este starea folosită de CI pentru execuția de referință.

## Configurația comună

Instanța de bază folosește `ell = 12` și `m = 40`. Cele 40 de ID-uri provin din `informatica/artifacts/dataset_split.json` și sunt aceleași ID-uri pe care CNN-ul le transformă în prefixe pentru HNP.

## Verificare

`verify_expected_vector` verifică faptul că cheia generată de experiment satisface embedding-ul. Această funcție verifică algebra, nu înlocuiește reducerea LLL. `validate_candidate` aplică limitele HNP pe toate eșantioanele.
