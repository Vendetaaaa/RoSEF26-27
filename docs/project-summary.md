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

Pentru evaluarea cantitativă, proiectul a rulat o matrice cu
`ell ∈ {8, 12}`, `sigma ∈ {0.10, 0.15, 0.20, 0.25}` și
`m ∈ {20, 40}`, pe trei seed-uri: `20260919`, `20260920`,
`20260921`.

## 4. HNP și LLL

În ultima etapă, rezultatele obținute sunt compactate în constrângeri
HNP, iar problema este formulată sub forma unui lattice, care poate fi
redus prin algoritmul LLL.

Evaluarea separă două cazuri. În cazul oracle, HNP primește prefixele
corecte ale nonce-urilor și măsoară recuperarea prin lattice fără eroarea
CNN. În cazul CNN → HNP, solverul primește prefixele produse de CNN.
Pentru `ell = 12` și `m = 40`, oracle a recuperat cheia în 3/3 rulări
la toate cele patru niveluri de zgomot. CNN → HNP a recuperat cheia în
2/3 rulări la `sigma = 0.10`, 1/3 la `sigma = 0.15` și 0/3 la
`sigma = 0.20` și `sigma = 0.25`.

Pentru `m = 20`, nu s-a observat recuperare în configurațiile testate.
Pentru `ell = 8` și `m = 40`, nu s-a observat recuperare nici în cazul
oracle, nici în cazul CNN → HNP.

Execuția de referință a solverului folosește backendul `fpylll`.
Rezultatele benchmarkului sunt păstrate în
`results/tables/cnn-hnp-parameter-sweep.csv` și
`results/tables/cnn-hnp-parameter-sweep-summary.csv`.
