# ESP32

Experimental ESP-IDF firmware for collecting ECDSA signing traces. It uses secp256k1, a controlled nonce, serial commands and a GPIO4 trigger. The firmware is for research measurements only, not production cryptography.

Build with ESP-IDF 5.x from `informatica/esp32/firmware` using `idf.py build`, then flash/monitor with `idf.py flash monitor`.
