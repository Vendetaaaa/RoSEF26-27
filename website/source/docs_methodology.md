# Metodologie

Metodologia separă trei componente care pot eșua independent: validarea algebrică, inferența CNN și recuperarea prin lattice. Fiecare etapă folosește artefacte identificabile și are propriul criteriu de verificare.

## ECDSA și HNP

Pentru fiecare semnătură se calculează termenii folosiți de formularea HNP:

```text
t_i = s_i^(-1) r_i mod n
u_i = s_i^(-1) z_i mod n
a_i = k_bar_i 2^(256-ell)
```

Ecuația HNP folosită de solver este:

```text
t_i d + u_i - a_i = delta_i (mod n)
```

cu:

```text
0 <= delta_i < B
B = 2^(256-ell)
```

Lattice-ul are dimensiunea `m + 2`. `HNPLatticeSolver` construiește embeddingul și verifică separat vectorul așteptat. Metoda `solve()` reduce baza, caută vectorul compatibil și validează candidatul obținut.

## CNN

`NonceBitCNN` primește două canale pentru trace-ul brut și diferența dintre eșantioane. Pentru configurația de bază, rețeaua prezice cei 12 biți ai prefixului MSB al nonce-ului.

Splitul explicit face ca notebook-urile și scripturile de producție să folosească aceleași ID-uri pentru train, validation și test. Cele 40 de ID-uri din test sunt apoi transmise etapei HNP.

## Legătura CNN → HNP

CNN-ul nu recuperează cheia privată direct. El furnizează informație parțială despre nonce, iar această informație este transformată în constrângeri HNP. Solverul lattice folosește constrângerile pentru a căuta candidatul pentru cheia privată.

Pentru a separa erorile, proiectul rulează și o instanță oracle. În oracle, prefixele nonce sunt cunoscute, astfel încât performanța lattice-ului poate fi măsurată fără incertitudinea CNN.

## Rezultatele matematice

`bridge_results.json` conține patru verificări numerice, iar `diophantine_results.json` conține încă cinci. Documentația păstrează aceste două grupuri separat de rezultatele atacului HNP.

Partea matematică riguroasă acoperă cazul scalar al concurenței pentru trei familii și transferul exponentului asociat. Experimentele pentru familii suplimentare pot susține explorarea numerică, dar nu transformă această explorare într-o teoremă pentru aproximarea simultană cu `k > 3`.

## Criterii de interpretare

`VALIDATED` în etapa HNP înseamnă că ecuațiile și limitele sunt compatibile cu cheia cunoscută. Nu înseamnă, singur, că atacul a recuperat cheia.

`PASS` la HNP cere un candidat returnat de LLL care trece validarea completă și, pentru execuția de referință, este obținut cu backendul `fpylll`.

Pentru benchmarkuri, o singură rulare nu este suficientă pentru o rată de succes. Matricea `(ell, sigma, m)` se rulează pe mai multe seed-uri și se raportează prin numărul de încercări și numărul de recuperări. Rezultatele includ separat oracle HNP și CNN → HNP.

## Limitele experimentului

Etapa ESP32 este păstrată separat până la obținerea unor traces fizice și a metadatelor necesare pentru validarea lor.
