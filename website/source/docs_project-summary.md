> **Stare metodologică (revizia curentă):** tabelele numerice de mai jos sunt artefacte istorice generate cu modelul sintetic anterior. Modelul implicit din cod a fost schimbat la un model temporal sintetic; aceste cifre nu au fost încă regenerate cu noua configurație și nu trebuie citate drept performanța curentă. Ele sunt păstrate pentru trasabilitate. Niciunul dintre aceste rezultate nu reprezintă recuperare validată din urme fizice ESP32.

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

Configurația de referință consideră disponibili primii 12 biți ai nonce-ului.
Setul public pentru atac și fișierele de ground truth au roluri diferite, dar
arhiva distribuită păstrează nonce-urile complete în
`informatica/artifacts/oracle_dataset.json`; câmpul explicit al cheii private
este redactat. Deoarece nonce-urile complete împreună cu semnăturile ECDSA
permit derivarea cheii sintetice, fișierul oracle este un artefact de evaluare,
nu o intrare validă pentru un atac orb. Pipeline-ul CNN → HNP trebuie să
folosească datele publice și prefixele prezise, iar ground truth-ul se folosește
numai după execuția solverului, pentru evaluare.

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

Sweep-ul de 100 de seed-uri arată diferența dintre oracle și prefixele prezise de CNN. Pentru `m = 40`, rezultatele de recuperare sunt prezentate mai jos.

**Notă despre timpii din sweep-ul arhivat:** rezultatele brute au fost generate de o versiune anterioară a orchestratorului, care verifica compatibilitatea cu cheia sintetică înainte de LLL. Codul curent rulează solverul înainte de verificarea cu ground truth. În fișierele istorice, timpii pentru instanțele sărite sunt condiționali și nu trebuie prezentați ca timpi end-to-end. Pentru comparația end-to-end curentă, consultați experimentul CNN ensemble + HNP robust din secțiunea următoare.

| Biți scurși `ell` | Oracle HNP | CNN → HNP pentru `sigma = 0, 0.05, 0.10, 0.15, 0.20` |
| ---: | ---: | --- |
| 8 | `0/100` în toate configurațiile | `0/100`, `0/100`, `0/100`, `0/100`, `0/100` |
| 10 | `100/100` în toate configurațiile | `61/100`, `51/100`, `47/100`, `6/100`, `0/100` |
| 12 | `100/100` în toate configurațiile | `69/100`, `44/100`, `33/100`, `4/100`, `0/100` |
| 14 | `100/100` în toate configurațiile | `70/100`, `49/100`, `40/100`, `3/100`, `0/100` |

Acuratețea CNN pe bit și pe prefix scade odată cu creșterea zgomotului. O acuratețe ridicată pe bit nu garantează un prefix complet corect, iar erorile de prefix afectează recuperarea HNP. Valorile din sweep rămân rezultate experimentale pentru configurațiile și seed-urile testate. Tabelul `cnn-hnp-baseline.csv` păstrează rezultatele baseline-ului, iar `hnp-oracle-thresholds.csv` conține rezultatele oracle pentru 400 de combinații.

Execuția de referință a solverului folosește backendul `fpylll`; artefactele raportate sunt în `informatica/artifacts/pipeline_results.json` și `informatica/artifacts/hnp_result.json`. Cheia privată este redactată în artefactele publice.

## 5. Evaluare comparativă: CNN individual, ensemble și HNP robust

Pentru a evalua separat contribuția combinării modelelor CNN și a
recuperării HNP robuste, a fost realizat un benchmark comparativ pe
100 de seed-uri distincte (`1200`–`1299`).

Configurația experimentului a fost: `ell = 12` biți ai prefixului
nonce, `sigma = 0.10`, `m = 40` semnături pentru atac, 160 de
eșantioane generate pentru fiecare seed și 100 de epoci de antrenare.
Pentru ensemble au fost antrenate cinci modele CNN cu inițializări
diferite, pe același split de date. Predicțiile lor au fost combinate
prin media probabilităților de clasificare. Primul model din ensemble
a fost utilizat și ca baseline CNN individual.

| Metodă | Chei recuperate | Rată de recuperare |
| --- | ---: | ---: |
| CNN individual + HNP standard | 43/100 | 43% |
| Ensemble CNN + HNP standard | 54/100 | 54% |
| Ensemble CNN + HNP robust | 90/100 | 90% |
| Oracle + HNP | 100/100 | 100% |

Acuratețea medie pe bit a fost de 99,758% pentru CNN individual
și 99,871% pentru ensemble. Acuratețea medie a prefixelor complete
a crescut de la 97,125% la 98,450%.

În configurația testată, ensemble-ul a crescut rata recuperării
cheii cu 11 puncte procentuale față de CNN individual.
Aplicarea recuperării HNP robuste a adus încă 36 de puncte
procentuale față de ensemble + HNP standard.

### 5.1. Recuperarea HNP robustă

Metoda robustă încearcă mai întâi recuperarea cheii utilizând
toate cele 40 de semnături. Dacă nu este identificată o cheie
validă, algoritmul încearcă succesiv subseturile obținute prin
eliminarea unei singure semnături.

Fiecare candidat recuperat este verificat prin comparație cu
cheia publică ECDSA. Cheia privată oracle nu este utilizată
pentru alegerea subseturilor sau pentru acceptarea candidatului
în această procedură.

În benchmarkul de 100 de seed-uri, metoda a recuperat cheia
în toate cele 54 de experimente cu 40/40 prefixe corecte și
în toate cele 36 de experimente cu 39/40 prefixe corecte.
Cele 10 eșecuri au corespuns unor experimente cu cel puțin
două prefixe greșite.

Algoritmul a efectuat în medie 12,11 încercări LLL, cu un
maximum de 41 de încercări. Timpul mediu pentru etapa HNP
robustă a fost de aproximativ 0,656 secunde per experiment,
fără includerea antrenării modelelor CNN.

### 5.2. Reproductibilitate și limitări

Benchmarkul a utilizat backendul `fpylll`. Rezultatele au fost
reproduse într-o a doua rulare, fără diferențe pentru cele
100 de seed-uri în câmpurile comparate privind prefixele
corecte, recuperarea cheii și încercările HNP robuste.

Fișierele experimentale sunt disponibile în:

- `results/validation/ensemble-robust-100-seeds/rosef-comparison-100-seeds.csv`
- `results/validation/ensemble-robust-100-seeds/rosef-comparison-100-seeds-summary.csv`

Rezultatele provin dintr-un model sintetic de scurgere de
informații, nu din măsurători fizice efectuate asupra unui
dispozitiv real. Rata de 90% descrie exclusiv configurația
și seed-urile evaluate, fără a constitui o garanție generală.

Metoda robustă testează eliminarea a cel mult unei semnături
și nu este concepută pentru a corecta arbitrar mai multe
prefixe greșite. Verificarea prin cheia publică reutilizează
implementarea aritmeticii eliptice din proiect; aceasta nu
reprezintă o verificare printr-o bibliotecă criptografică
independentă.

## Validare suplimentară: model temporal și sensibilitatea HNP

Modelul temporal sintetic folosește relația:
L_i = 0.2*b_i + 0.6*b_(i-1) + 0.2*b_(i-2) + epsilon_i,
cu zgomot gaussian de deviație standard 0.15.

Benchmarkul folosește 160 de exemple ECDSA, împărțite în
96 train, 24 validation și 40 test. Cele 100 de seed-uri
(1400–1499) modifică zgomotul și inițializarea CNN, nu cheia
privată sau împărțirea datasetului.

### Comparația metodelor

| Metodă | Acuratețe biți | Prefixe exacte |
|---|---:|---:|
| Threshold | 54.41% | 0.00% |
| Viterbi | 93.95% | 56.55% |
| CNN raw | 85.66% | 12.85% |
| CNN diff | 69.49% | 2.02% |
| CNN both | 88.86% | 24.95% |

Viterbi utilizează coeficienții cunoscuți ai modelului
temporal, reprezentând un baseline informat despre canal.

### Sensibilitatea HNP la erori

În experimentul cu 40 de semnături și 12 biți MSB:
- 0 prefixe greșite: standard 10/10, robust 10/10;
- 1 prefix greșit: standard 0/10, robust 10/10;
- 2 sau mai multe prefixe greșite: ambele metode 0/10
  în configurațiile testate.

### Selecția după încrederea CNN

Pentru 20 de semnături, selecția după încredere a obținut
35.30% prefixe exacte, comparativ cu 23.80% pentru
selecția aleatoare. Îmbunătățirea nu a fost suficientă
pentru recuperarea cheii în configurația temporală evaluată.

Aceste rezultate sunt distincte de experimentul cu
ensemble CNN și HNP robust pe modelul cumulativ,
unde recuperarea cheii a reușit în 90/100 cazuri.
