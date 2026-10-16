# Revizie diagnostic INA219 — RO

## Ce s-a corectat

1. **Calibrare pentru curentul observat.** Driverul folosește profilul Adafruit `32V/1A` în loc de `32V/2A`. Cu shunt-ul standard de 0,1 Ω, profilul oferă pas de curent de 0,04 mA și pas de registru de putere de 0,8 mW. Păstrează intervalul de magistrală de 32 V. Folosește-l doar dacă modulul chiar are shunt de 0,1 Ω și curentul maxim rămâne sub 1 A.

2. **Citiri de pornire eliminate.** După configurare, driverul așteaptă 10 ms și citește o dată fiecare registru, verificând rezultatul tranzacțiilor I²C. `FINAL` mai elimină două citiri de încălzire și raportează media a cinci eșantioane de bază, min/max și deviația standard a curentului. Asta ajută la detectarea primei citiri instabile, cum era valoarea de 0 mA din jurnal.

3. **Două estimări de putere.** `P_MW` rămâne valoarea registrului de putere INA219; `P_CALC_MW` este calculată separat ca `V_BUS × I_MA`. Registrul este cuantizat de profilul de calibrare, astfel încât cele două valori nu trebuie să coincidă exact.

4. **Planificarea eșantionării.** Task-ul folosește `vTaskDelayUntil`, ceea ce evită adăugarea integrală a timpului de tranzacție I²C peste perioada nominală de 20 ms. Raportul `FINAL` publică intervalul minim, maxim și mediu măsurat dintre timestamp-uri.

5. **Comparație baseline/activ mai puțin contaminată.** Diferența de curent este calculată din citirile ale căror tranzacții I²C sunt integral în fereastra trigger LOW, comparate cu citiri complet anterioare. Citirile care traversează fronturile sunt numărate separat, nu amestecate în media activă.

## Cum testezi

1. Instalează biblioteca Arduino **Adafruit INA219** cerută de proiect.
2. Deschide `main/main.ino` în Arduino IDE și încarcă firmware-ul.
3. Deschide Serial Monitor la 115200 baud, pornește PulseView pe GPIO4 și trimite `FINAL`.
4. Verifică `BASELINE_I_MA`, `BASELINE_I_MIN_MA`, `BASELINE_I_MAX_MA`, `BASELINE_I_STD_MA`, `BASELINE_P_MW`, `BASELINE_P_CALC_MW`, `SAMPLE_INTERVAL_*_US` și liniile `TRACE`.
5. Compară `P_MW` și `P_CALC_MW`. Dacă `P_MW` se schimbă în pași, dar `P_CALC_MW` este mai fin, diferența vine din cuantizarea registrului; dacă prima citire rămâne 0 mA sau valorile sunt incoerente, verifică shunt-ul, conexiunile și calibrarea hardware.

## Limite

Această revizie îmbunătățește validitatea citirilor și sincronizarea software. **Nu implementează recuperarea unei chei private** și nu transformă INA219 într-un ADC de formă de undă rapidă. `FINAL` continuă să execute exact o operație cu cheia de test, nu exportă nonce-ul și nu declară cheia recuperată. Pentru a evalua scurgerea fizică rapidă este nevoie de un digitizor analogic potrivit; PulseView cu un analizor logic observă semnalul digital GPIO4, nu forma analogică a curentului.

## Validare software

Testele din `main/tests/test_source_contract.py` verifică contractele comenzilor și noile rapoarte statice. Ele nu înlocuiesc o compilare cu Arduino toolchain și un test fizic pe placă.

## Corecție de memorie RAM pentru compilarea ESP32

Versiunea V3 inițială stoca în fiecare eșantion atât timestamp-ul de mijloc, cât și puterea calculată, deși ambele sunt valori derivabile. Pentru 2048 de eșantioane, aceste câmpuri redundante creșteau `.dram0.bss` cu aproximativ 16 KiB și puteau cauza eroarea linkerului `dram0_0_seg overflowed`.

Structura a fost optimizată fără a micșora numărul de eșantioane și fără a elimina măsurătorile păstrate: startul și finalul tranzacției I²C rămân stocate; timestamp-ul de mijloc este calculat din ele când se folosește, iar `P_CALC_MW` este calculată la raportare ca `V_BUS × I_MA`. Se păstrează tensiunea shunt, curentul și puterea din registru. Dimensiunea structurii revine la dimensiunea variantei anterioare care compila.

Această corecție este verificată cu teste statice de contract. Compilarea Arduino/ESP32 trebuie confirmată pe mediul care a produs eroarea.
