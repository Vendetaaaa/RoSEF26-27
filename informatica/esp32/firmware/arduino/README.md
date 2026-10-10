# Arduino ESP32 firmware

This directory contains the Arduino-compatible firmware used for the RoSEF-26/27 ESP32 experiments.

## Logic Analyzer / BUFA trigger

`GPIO 4` is the Logic Analyzer trigger and is intentionally **HIGH at idle**.
The firmware pulls it **LOW only while the ECDSA signing operation is running**,
then returns it to HIGH. This avoids holding an active-HIGH BUFA input LOW when
the wire is connected.

## INA219 serial commands

Commands are case-sensitive and must be uppercase. The INA219 sensor uses I2C
on SDA GPIO 21 and SCL GPIO 22.

- `INA219` reads one sample. If the sensor is not detected, the command returns
  `ERR INA219_UNAVAILABLE` without blocking the serial command loop.
- `BATCH INA219 <quant>` reads the sensor the requested number of times, from 1
  through 1000 samples. Example: `BATCH INA219 100`.
- `BATCH <count>` remains the existing ECDSA batch command.

Install the **Adafruit INA219** Arduino library before compiling.


## FULLIMP v2 integrated capture

`FULLIMP` is a test-only diagnostic. It requires the stock `TEST_PRIVATE_KEY` and
`TEST_DEFAULT_NONCE` vectors; if `SETD` or `SETK` has changed those values, reboot
the board to restore the test vectors before running `FULLIMP`. The command refuses
to begin its signatures unless those vectors match, so the fixed test nonce is not
used with a non-test key.

The command performs the existing self-test, checks INA219, runs a sign-and-verify
probe, captures a trigger-idle INA219 baseline, then performs 100 ECDSA signatures
with independent ECDSA verification. A background FreeRTOS task polls INA219 while
the active-LOW trigger on GPIO 4 is asserted. It emits timestamped `SAMPLE OP=...`
records and per-operation `BATCH n: ... INA219: ...` summaries. The INA219 sample
interval is approximate: each sample includes I2C transaction latency and is not a
high-speed instantaneous current waveform. Check the physical trigger/capture in
PulseView; firmware cannot determine whether a Logic Analyzer is attached.

The nominal 330-ohm current model is a comparison estimate only, not a calibration.
The legacy `BATCH <count>` and `BATCH INA219 <count>` commands are unchanged.
