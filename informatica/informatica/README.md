# Partea informatică a acestui proiect

1. Scopul final al acestui cod constă în crearea unei rețele neuronale și antrenarea ei folosind diverse exemple.

2. Primul pas a reprezentat cea mai dificilă etapă în scrierea codului. Precum un geograf nu își poate găsi drumul fără o hartă, așa un informatician nu își poate constrânge ideile de atac fără un plan.

   i. Întâi am realizat un cod simplist pentru a asigura succesiunea fluidă a următoarelor sesiuni de cod și pentru a organiza etapele proiectului într-o ordine clară.

   ii. Baza conglomeratului aritmetic este înființată de algoritmul ECDSA (Elliptic Curve Digital Signature Algorithm). Această bază a fost construită integral în Python, fără a folosi biblioteci criptografice externe, precum OpenSSL sau cryptography. Codul generează chei, realizează verificarea semnăturilor, cât și o demonstrație practică a unui atac criptanalitic clasic: vulnerabilitatea cauzată de reutilizarea aceluiași număr aleator (nonce sau k).

   iii. Următoarea etapă constă în generarea unui set de date sintetice pentru benchmark-ul semnăturilor ECDSA și al problemei HNP.

   *benchmark = set de date standardizat și controlat.*

   Rolul benchmark-urilor semnăturilor ECDSA constă în simularea unei vulnerabilități existente în viața reală. Dacă un program generează numerele aleatoare (nonces k) cu o mică slăbiciune, cum ar fi câțiva biți cunoscuți sau o altă informație parțială despre nonce din cauza unei erori de implementare, un atacator poate folosi metode matematice avansate, cum sunt atacurile pe rețele (lattice attacks) folosind algoritmul LLL, pentru a încerca recuperarea cheii private a unui utilizator.

   Codul implementează curba eliptică `secp256k1` într-un mediu controlat, cu scop experimental.

   Semnăturile ECDSA sunt formate din perechi de forma `(r, s)`, pe baza unui mesaj `(z)`, a unei chei private și a unui număr aleatoriu ales, precum `(k)`.

   Codul separă datele publice de cele Ground Truth. Cum funcționează acest lucru?

   * Datasetul public conține doar informații pe care un algoritm de atac le-ar avea la dispoziție (ex. mesaj, semnătura `r` și `s`, hash-ul `z` și informația despre biții de scurgere ai nonce-ului).
   * Ground Truth păstrează separat cheia privată și valorile complete ale fiecărui nonce `(k)`.

   iv. După ce au fost generate semnăturile ECDSA, acestea sunt asociate cu un model sintetic de leakage, cu scop experimental.

   Cel mai important aspect al acestei etape este modelarea scurgerilor de putere. Când un procesor sau microcontroler calculează operații criptografice, consumul lui de energie electrică poate varia în funcție de datele pe care le prelucrează.

   Scriptul folosește modele matematice precum **Hamming Weight (HW)** și **Hamming Distance (HD)** pentru a modela relația dintre datele procesate și variațiile de consum.

   Mai departe se adaugă un zgomot de măsurare (**Gaussian Noise**). De ce?

   În lumea reală, măsurătorile fizice ale consumului de curent nu sunt perfecte, ci conțin zgomot electric și alte variații. Așadar, scriptul adaugă zgomot gaussian pentru a face simularea mai apropiată de condițiile unui experiment fizic.

   Programul încearcă să determine informația asociată biților nonce-ului, fiecare bit având una dintre cele două valori posibile, `0` sau `1`. Scopul este ca informația care poate fi observată în trace să poată fi transformată într-o informație utilă pentru etapele următoare ale atacului.

   v. Pentru a reuși să recuperăm cu adevărat o cheie secretă, vom construi o rețea neuronală (**CNN 1D**) cu ajutorul metodelor de **Deep Learning**. Acest script analizează semnalele de curent, transformându-le în șiruri de valori. Rețeaua învață să recunoască modele de consum asociate cu datele procesate, trecând prin mai multe etape de clasificare și transformare a informației.

   În etapa experimentală actuală, una dintre sarcinile rețelei este clasificarea informației de tip **Hamming Weight** pentru date de dimensiune mică, de la `0` la `8`, folosind exclusiv forma trace-ului de curent. Această etapă este folosită pentru a verifica dacă rețeaua poate extrage informație relevantă dintr-un semnal zgomotos.

   Rețeaua neuronală împarte datele în seturi de antrenare și validare, rulează mai multe epoci de învățare, iar în final afișează acuratețea cu care inteligența artificială a reușit să ghicească informația. În etapele următoare, informația extrasă de rețea va fi folosită pentru recuperarea unor biți ai nonce-ului necesari atacului HNP.

   vi. Ultima piesă de puzzle este atacul matematic (**Lattice Attack**), care încearcă recuperarea unei chei private ECDSA folosind informațiile parțiale scurse despre nonce.

   Scriptul transformă ecuațiile de semnătură ECDSA într-un sistem de ecuații liniare în care valorile necunoscute ale nonce-urilor sunt legate de cheia privată secretă. Acesta construiește o matrice de rețea (**Kannan Embedding Lattice Basis**) care introduce datele publice, precum mesajele, semnăturile și biții divulgați.

   Apoi se aplică algoritmul **LLL (Lenstra–Lenstra–Lovász)**, care reduce baza rețelei și produce vectori mai potriviți pentru identificarea soluției. Din vectorii obținuți se caută un candidat pentru cheia privată, iar candidatul este verificat pentru a vedea dacă este compatibil cu datele originale ale sistemului.

   vii. Experimentele diofantine reprezintă niște sisteme de testare pentru a valida și a analiza proprietăți matematice în domeniul geometriei numerice. În această etapă, codul împletește ecuațiile matematice teoretice cu comportamentul lor real atunci când sunt simulate pe un calculator.

   Printre exemple se află legătura dintre teoria numerelor, teorema Kronecker–Weyl, teorema Steinhaus–Sós a celor 3 distanțe și distribuția punctelor în spațiu.

   viii. Ca toate finalurile apoteotice ale romanelor de dragoste, codul nostru se încheie oficial prin demonstrarea faptului că ecuațiile matematice teoretice sunt pe aceeași lungime de undă cu simulările realizate pe calculator.

   Cum?

   Așadar, scriptul implementează 4 module de legătură între partea teoretică și experimentele numerice, afișând rezultatele printr-un tablou de bord standardizat:

   1. **Puntea factorului asimptotic Kronecker–Weyl**
      Calculează defectul de proximitate pe un plan finit pentru numere iraționale și verifică dacă rezultatul experimental se apropie de constanta analitică teoretică.

   2. **Transferul exponentului diofantic**
      Studiază relația dintre exponenții clasici și exponenții geometrici de „aproape-concurență”, folosind regresii liniare în coordonate `log-log` pentru a verifica ratele de convergență.

   3. **Puntea de rigiditate Steinhaus–Sós a celor 3 distanțe**
      Analizează cum se împart distanțele într-o secvență numerică și verifică proprietatea teoretică privind numărul de distanțe distincte.

   4. **Dicționarul de închidere**
      Testează o rezonanță cu pante raționale pentru cazul `k = 3`. Compară încercările efectuate prin intermediul unei scanări la nivelul intersecțiilor de linii și verifică dacă ambele metode găsesc același număr de triplete. În varianta corectată a experimentului, rezultatul este de **321 de soluții**, cu o diferență simetrică nulă.
