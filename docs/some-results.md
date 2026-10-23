## Întrebarea de cercetare

Întrebarea investigată este dacă un model de învățare automată poate extrage indicii utile despre nonce-ul ECDSA din urme side-channel și dacă aceste indicii pot fi folosite de etapa HNP/LLL pentru a îmbunătăți probabilitatea de recuperare a cheii în condiții experimentale definite.

Evaluarea trebuie să distingă între:

1. predicția unor biți sau caracteristici pe date sintetice;
2. performanța întregului pipeline, de la intrarea în model până la verificarea recuperării;
3. indicatorii derivați din metadate hardware;
4. recuperarea confirmată cap-coadă pe urme fizice reale.

Acestea sunt niveluri diferite de evidență. Reușita într-un nivel nu demonstrează automat reușita în următorul.

## Rezultate sintetice

În experimentele sintetice efectuate, echipa raportează o rată de succes de aproximativ **89–95,5%**, în funcție de configurația evaluată. Datele sintetice permit controlul condițiilor și repetarea experimentelor, fiind utile pentru verificarea logicii pipeline-ului și pentru compararea variantelor sale.

În experimentele anterioare a fost folosit și un parametru de zgomot sintetic notat **σ = 0,1**. Acesta trebuie prezentat drept parametru al simulării numai în măsura în care reprezintă într-adevăr nivelul de zgomot din configurația raportată. Nu trebuie confundat cu o marjă statistică de ±0,1 puncte procentuale. Intervalul 89–95,5% este descris aici ca interval al rezultatelor observate raportate de echipă, nu ca interval de încredere calculat.

## Rezultatul preliminar din metadatele hardware

Pentru configurația fizică descrisă ca „fără osciloscop, cu INA”, echipa a calculat un indicator de succes de aproximativ **39,4%**, folosind metadatele disponibile.

Acest rezultat trebuie descris ca **estimare preliminară bazată pe metadate**, deoarece metadatele unui senzor de monitorizare a tensiunii/curentului nu sunt, prin ele însele, echivalente cu o urmă electrică de mare rezoluție, sincronizată cu operațiile criptografice. Pentru a afirma că pipeline-ul recuperează cheia din urme fizice, este necesară evaluarea pe semnale măsurate și sincronizate, trecute prin întregul pipeline, cu cheia verificată independent la final. ( Calculat drept fiind, este o confirmare drept pipleline reușit end-to-end )


## Informații omise intenționat din versiunea publică

| 1 | Tabele complete, pe fiecare rulare și seed, pentru benchmark-ul sintetic 

| 2 | Seturile brute sintetice care includ nonce-uri, chei de referință sau alte valori de tip ground truth

| 3 | Parametrii interni detaliați ai modelului și pragurile de decizie

| 4 | Parametrii compleți ai etapei HNP/LLL și logurile interne ale solverului

| 5 | Urmele brute de tensiune/curent și metadatele temporale detaliate ale sesiunilor hardware

| 6 | Detalii complete de sincronizare, cablare și configurare a instrumentației

| 7 | Setările operaționale interne și procedurile de test ale configurației Beta 1.5V 

| 8 | Logurile complete de depanare, erorile intermediare și notele interne de calibrare | Evitarea confuziei între teste exploratorii și rezultate validate

Rezultatele raportate susțin fezabilitatea explorării pipeline-ului pe date sintetice, unde rata de succes observată este de aproximativ **89–95,5%** în configurațiile testate. Analiza metadatelor din configurația descrisă ca „fără osciloscop, dar cu INA219” a generat un indicator preliminar de aproximativ **39,4%**.

Documentația publică omite selectiv unele tabele și detalii de implementare pentru protejarea configurației Beta 1.5V și a confidențialității concursului, păstrând totodată rezultatele agregate, limitările și condițiile necesare unei evaluări corecte.
