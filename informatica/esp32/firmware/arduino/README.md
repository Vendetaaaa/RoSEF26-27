# Arduino ESP32 firmware

This directory contains the Arduino-compatible firmware used for the RoSEF-26/27 ESP32 experiments.

## Logic Analyzer / BUFA trigger

`GPIO 4` is the Logic Analyzer trigger and is intentionally **HIGH at idle**.
The firmware pulls it **LOW only while the ECDSA signing operation is running**,
then returns it to HIGH. This avoids holding an active-HIGH BUFA input LOW when
the wire is connected.
