# Întrebarea de cercetare

Proiectul testează dacă informația parțială și zgomotoasă despre nonce-ul ECDSA poate fi transformată, într-un experiment controlat, în informație suficientă pentru recuperarea verificabilă a cheii private.

Întrebarea computațională principală este:

> Pentru semnături ECDSA pe secp256k1 generate de echipă, câtă informație despre nonce trebuie extrasă din urme sintetice pentru ca un CNN să producă prefixe suficient de precise, iar HNP & LLL să poată recupera cheia privată?

Experimentul de bază fixează primii 12 biți MSB ai nonce-ului, folosește 40 de semnături pentru instanța HNP și păstrează o împărțire explicită a celor 160 de eșantioane: 96 train, 24 validation și 40 test/attack.

Proiectul separă trei niveluri de verificare:

1. corectitudinea ECDSA și a ecuațiilor HNP;
2. reducerea lattice-ului prin LLL și validarea candidatului;
3. recuperarea cheii folosind prefixele produse de CNN.

Instanța oracle folosește prefixele cunoscute pentru a testa solverul HNP fără eroarea introdusă de CNN. Această verificare spune dacă problema lattice poate fi rezolvată în condițiile date, dar nu demonstrează că CNN poate produce aceleași prefixe.

Pentru partea CNN, benchmarkul variază `ell`, `sigma` și `m` și separă acuratețea predicțiilor de succesul HNP. Astfel, o eroare de clasificare nu este confundată cu o eroare a solverului lattice.

Partea matematică are o limită explicită. Rezultatul riguros este formulat pentru cazul scalar cu trei familii și transferul exponentului asociat. Extinderea la aproximarea simultană pentru mai mult de trei familii rămâne neacoperită de demonstrația actuală.

Etapa hardware rămâne separată de rezultatele curente. Repository-ul păstrează firmware-ul și protocolul ESP32, dar proiectul nu prezintă măsurători fizice până când acestea nu există.


## Model de scurgere și stare curentă

Modelul implicit din `ecdsa_leakage_model4.py` este acum un model temporal sintetic cu memorie de două poziții, nu modelul HW pe prefix care permitea identitatea exactă `HW(prefix_t) - HW(prefix_{t-1}) = b_t`. Modelul temporal este o abstracție de cercetare, nu un model validat al consumului ESP32. Diferențele dintre eșantioane pot conține în continuare informație despre etichetă; de aceea trebuie raportate baseline-uri simple și evaluare pe date independente.

Rezultatele istorice generate cu modelul HW sunt păstrate pentru trasabilitate, dar nu trebuie citate drept rezultate ale configurației implicite actuale. Regenerați datasetul de profil, antrenarea CNN, predicțiile și toate benchmarkurile înainte de a raporta metrici noi.

Contribuția informatică evaluată aici este una experimentală: comparația dintre un model temporal sintetic, clasificarea biților/prefixelor și rezolvarea HNP/LLL în condiții controlate. Nu se afirmă recuperarea unei chei dintr-o urmă fizică ESP32 până când urmele analogice brute și rezultatele end-to-end nu sunt incluse și verificate.
