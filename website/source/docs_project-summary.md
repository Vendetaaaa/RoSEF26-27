# Rezumatul proiectului

Proiectul nostru urmărește recuperarea informației parțiale despre
nonce-ul secret utilizat în timpul generării semnăturilor ECDSA. Prin
generarea unor semnale artificiale zgomotoase, modelăm, într-o formă
simplificată, informația care ar putea fi observată printr-un canal
lateral. Biții parțial recuperați sunt transformați în informație utilă
pentru formularea unei probleme Hidden Number Problem (HNP).

Cercetarea combină două domenii: matematica și informatica. Acestea
construiesc un lanț experimental care pornește de la teoria aproximării
diofantice și ajunge la simularea unei analize de tip side-channel,
clasificarea semnalelor și formularea unei probleme HNP.

## 1. Matematica

La baza codului și a algoritmilor se află matematica. Problema
geometrică este transformată într-o problemă aritmetică prin relația
(p = Rq), unde (p) și (q) reprezintă parametrii care descriu dreptele, iar
(R) este raportul dintre pantele acestora. Atunci când acest raport este
rațional, ecuația admite familii de soluții întregi, ceea ce corespunde
unor configurații în care dreptele se intersectează exact într-un punct
comun.

În cazul în care raportul este irațional, o astfel de concurență exactă
nu apare, iar proiectul analizează cât de aproape pot ajunge dreptele de
această situație. Pentru descrierea acestei apropieri sunt introduse
concepte precum defectul de concurență, aproximarea diofantică și viteza
cu care soluțiile se apropie de valorile ideale. Rezultatele teoretice
sunt apoi verificate prin experimente numerice, fiind analizate
constantele asimptotice, transferul exponentului diofantic, proprietatea
celor trei distanțe și legătura dintre soluțiile aritmetice obținute și
pozițiile intersecțiilor geometrice.

## 2. Informatica și criptografia

Simularea criptografică folosește algoritmul ECDSA pe curba secp256k1.
Fiecare experiment utilizează semnături digitale, iar fiecare semnătură
are asociate un mesaj, un hash, valorile (r) și (s) și un nonce secret.

Pentru experiment se consideră disponibili primii 12 biți ai nonce-ului.
Datele publice sunt separate de o barieră care păstrează cheia privată
și nonce-urile complete, astfel încât acestea să poată fi folosite numai
pentru verificarea experimentală.

## 3. Zgomot Gaussian

După generarea semnăturilor, sunt construite semnale artificiale pe baza
modelului Hamming Weight. Zgomotul Gaussian este adăugat controlat
pentru a reproduce, într-o formă simplificată, imperfecțiunile întâlnite
în măsurătorile fizice.

Pentru configurația de bază (`sigma = 0.15`), rularea de referință cu
100 de epoci a obținut 99,65% acuratețe pe bit și 95,83% acuratețe pe
prefix pe validation, respectiv 100,00% și 100,00% pe test (40/40).

Pentru evaluarea cantitativă, proiectul a rulat 20 de configurații: `ell ∈ {8, 10, 12, 14}`, `sigma ∈ {0, 0.05, 0.10, 0.15, 0.20}` și `m = 40`. Fiecare configurație agregă 100 de seed-uri, de la `1000` la `1099`, cu 100 de epoci de antrenare. Rezultatele sunt în `results/tables/cnn-hnp-parameter-sweep-summary.csv`.

## 4. HNP și LLL

În ultima etapă, prefixele nonce devin constrângeri HNP, iar problema este formulată ca un lattice redus cu LLL. Rularea de referință pentru configurația de bază este înregistrată ca `PASS`: solverul a folosit `fpylll`, a recuperat cheia sintetică și a validat-o față de ground truth. Acest rezultat se referă la instanța de bază, nu la fiecare configurație din benchmark.

Sweep-ul de 100 de seed-uri arată diferența dintre oracle și prefixele prezise de CNN. Pentru `m = 40`, rezultatele de recuperare sunt:

| Biți scurși `ell` | Oracle HNP | CNN → HNP pentru `sigma = 0, 0.05, 0.10, 0.15, 0.20` |
| ---: | ---: | --- |
| 8 | `0/100` în toate configurațiile | `0/100`, `0/100`, `0/100`, `0/100`, `0/100` |
| 10 | `100/100` în toate configurațiile | `61/100`, `51/100`, `47/100`, `6/100`, `0/100` |
| 12 | `100/100` în toate configurațiile | `69/100`, `44/100`, `33/100`, `4/100`, `0/100` |
| 14 | `100/100` în toate configurațiile | `70/100`, `49/100`, `40/100`, `3/100`, `0/100` |

Acuratețea CNN pe bit și pe prefix scade odată cu creșterea zgomotului. O acuratețe ridicată pe bit nu garantează un prefix complet corect, iar erorile de prefix afectează recuperarea HNP. Valorile din sweep rămân rezultate experimentale pentru configurațiile și seed-urile testate. Tabelul `cnn-hnp-baseline.csv` păstrează rezultatele baseline-ului, iar `hnp-oracle-thresholds.csv` conține rezultatele oracle pentru 400 de combinații.

Execuția de referință a solverului folosește backendul `fpylll`; artefactele raportate sunt în `informatica/artifacts/pipeline_results.json` și `informatica/artifacts/hnp_result.json`. Cheia privată este redactată în artefactele publice.

