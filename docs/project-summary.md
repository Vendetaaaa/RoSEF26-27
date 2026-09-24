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
geometrică este transformată într-o problemă aritmetică prin relația (p
= Rq), unde (p) și (q) reprezintă parametrii care descriu dreptele, iar
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

Pentru zgomotul de referință (`\sigma `{=tex}= 0{,}15), algoritmul
obține aproximativ 99,79% acuratețe pe bit și 98,75% dintre prefixele de
12 biți recuperate integral, conform experimentului de referință.

## 4. HNP și LLL

În ultima etapă, rezultatele obținute sunt compactate în constrângeri
HNP, iar problema este formulată sub forma unui lattice, care poate fi
redus prin algoritmul LLL.

Relațiile HNP sunt verificate folosind 40 de semnături, concomitent cu
valorificarea și validarea experimentelor matematice. Cheia privată
recuperată este apoi comparată cu cheia de referință, iar rezultatul
este înregistrat împreună cu acuratețea programului.
