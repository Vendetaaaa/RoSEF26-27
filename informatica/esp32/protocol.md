# ESP32 protocol

Minimal serial protocol for the experimental ECDSA firmware.

- `PING` → `OK PONG`
- `INFO` → device/configuration information
- `SETK <64 hex>` → set controlled 32-byte nonce
- `SETD <64 hex>` → set experimental private key
- `SIGN <text>` → SHA-256(text), trigger, ECDSA sign, return `R` and `S`
- `SIGNHEX <64 hex>` → sign a supplied SHA-256 digest
- `HELP` → commands

The trigger is GPIO4 and is HIGH only during the signing operation. `SETK`/`SETD` are for controlled experiments and must not be used with real secrets.
