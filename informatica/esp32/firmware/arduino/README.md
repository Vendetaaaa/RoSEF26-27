# Arduino ESP32 firmware

This directory contains the Arduino-compatible firmware used for the RoSEF-26/27 ESP32 experiments.

## Logic Analyzer / BUFA trigger

`GPIO 4` is the Logic Analyzer trigger and is intentionally **HIGH at idle**.
Signing commands can pull it LOW around ECDSA work. `SBATCH` is deliberately a
separate, minimal GPIO-only diagnostic: it produces a visible train of LOW/HIGH
pulses and does not run ECDSA or INA219. Use it to prove the GPIO4 wire and
PulseView channel work before running a cryptographic capture.

## INA219 serial commands

Command keywords are case-insensitive (`PING`, `Ping`, and `ping` work). The
message text after `SIGN` preserves its case. Commands are entered in Arduino
Serial Monitor at 115200 baud; choose the `Both NL & CR` line ending. The INA219
sensor uses I2C on SDA GPIO 21 and SCL GPIO 22.

- `INA219` reads one sample. If the sensor is not detected, the command returns
  `ERR INA219_UNAVAILABLE` without blocking the serial command loop.
- `BATCH INA219 <quant>` reads the sensor the requested number of times, from 1
  through 1000 samples. Example: `BATCH INA219 100`.
- `BATCH <count>` remains the existing ECDSA batch command.
- `SBATCH [count]` is a GPIO4-only signal generator for PulseView. It emits `count` cycles of LOW for 250 ms then HIGH for 250 ms (default count 10; maximum 1000). It deliberately does not call ECDSA, BATCH, or INA219.


Example command for a clearly visible GPIO4 pulse train:

- `SBATCH 10` — generate ten LOW/HIGH cycles, each level lasting 250 ms. `GPIO4` returns to HIGH when the command finishes. A bare `SBATCH` also generates ten cycles.

Install the **Adafruit INA219** Arduino library before compiling.


## SCOPECAP / FULLIMP test capture

`SCOPECAP` runs 160 ECDSA signatures with the stock `TEST_PRIVATE_KEY` and
fresh random nonces. It refuses to start if `SETD` or `SETK` changed the stock
test vectors; reboot the board to restore them. The trigger is suppressed during
self-tests and preflight, so a successful `SCOPECAP` emits exactly 160 LOW
pulses on GPIO4, in the same order as `OPMETA OP=1..160`. It does not run the
background INA219 task, avoiding extra I2C activity during the analog capture.
The `OPMETA ... K_GT=...` nonce is a test-only ground-truth label; it must not be
included in CNN input features.

`FULLIMP` is the separate INA219 diagnostic mode. It timestamps the beginning,
midpoint and end of each I2C read on the ESP32 `micros()` timebase and associates
readouts to trigger windows after the batch. INA219 values are slow register
readouts—not a high-speed, instantaneous current waveform—and do not, by
themselves, prove nonce leakage.

For CNN-ready waveforms, capture the analog current signal and GPIO4 trigger
with a high-speed oscilloscope/DAQ. Export one CSV per operation with
`time_s,current_ma,trigger_v`, including a HIGH trigger level both before and
after the active-LOW pulse. The RoSEF tools import these CSVs, keep ECDSA public
metadata separate from nonce labels, train/evaluate a physical-trace CNN, and
pass only test-split CNN prefixes to HNP. A firmware `OK SCOPECAP=PASS` or
`OK FULLIMP=PASS` validates firmware checks only; neither claims that the
oscilloscope produced good traces, the CNN recovered nonce bits, or HNP
recovered a key.


### INA219 RAM-safe capture configuration (FIX2)

FULLIMP V3 uses a 2048-entry sample buffer and a 20 ms sampler delay to keep the global DRAM footprint within the standard ESP32 RAM budget. INA219 sampling is paused during ECDSA verification and enabled only for the trigger-active signing interval and the idle baseline. The sampler uses a fixed 20 ms start-to-start target, with observed interval jitter and I2C transaction durations reported by `FINAL`. This is a low-rate diagnostic trace, not a fast waveform capture. If overflow is reported, treat that run as invalid; do not use it for CNN/HNP.

### Trigger check in PulseView

Use `PULSE 250000` to emit one diagnostic GPIO4 pulse: idle HIGH, LOW for about
250 ms, then HIGH again. `OK PULSE WIDTH_US=...` reports elapsed software time,
not external analyzer confirmation. For a more reliable wiring/capture check, arm
PulseView first, then send `TRIGTEST` (or `TRIGTEST 8`): it emits eight repeated
LOW/HIGH cycles, each about 250 ms per level, and reports the MCU pad read-back.
A `TRIGTEST=PASS` read-back still does not prove the external analyzer is connected;
the captured D0 waveform must show the edges. For `FULLIMP`, GPIO4 is idle HIGH
and goes LOW for each ECDSA signing operation (about 242 ms in the supplied run),
then returns HIGH; operations are spaced roughly 0.95 s apart. Share GND between
the ESP32 and logic analyzer and connect analyzer D0 to the board pin labelled
GPIO4/IO4.

## Command-audit fixes (2026-10-11)

The command layer and shared trigger handling include additional guards:

- `BATCH <count>` runs each ECDSA signature, verifies it, and reports per-operation
  `TRIGGER_LOW_READBACK` / `TRIGGER_HIGH_READBACK`; the final summary counts all
  verified signatures and GPIO read-backs. For example, `BATCH 35` must end with
  `SUMMARY COUNT=35 VERIFY_PASS_COUNT=35 TRIGGER_READBACK_PASS_COUNT=35` and
  `OK BATCH=PASS`.
- `TRIGTEST [count]` emits repeated, easy-to-observe 250 ms LOW / 250 ms HIGH
  pulses on GPIO4 (`count` 1..12; default 8). Start PulseView acquisition before
  sending this command.
- `PULSE [microseconds]` reports firmware read-backs for both LOW and HIGH.
  These read-backs validate the ESP32 pin state only; they do not prove the wire
  reaches the logic-analyzer channel.
- `FULLIMP` and `SCOPECAP` now include LOW/HIGH GPIO read-back status per operation
  and fail their summary if those firmware checks fail.
- `SELFTEST` suppresses trigger pulses so self-test signatures are not mistaken
  for labelled operations.
- `TESTPR` restores GPIO27 to the button's `INPUT_PULLUP` mode when finished.
- `SETK` and `SETD` validate input in a temporary buffer and only commit valid
  secp256k1 scalars; malformed commands cannot partially corrupt the live value.
- overlong serial input is discarded through its newline instead of allowing its
  tail to be parsed as a separate command.

### Complete command checklist (source of truth: `handleLine()`; expanded catalog: `docs/COMMAND_MENU_COMPLETE_RO.md`)

The Serial Monitor prints this complete categorized menu after boot/reset, and
`HELP`, `MENU`, or `?` prints it again. Every documented command form below is
routed by the protocol parser:

| Command | Purpose / accepted arguments |
|---|---|
| `HELP`, `MENU`, `?` | Display the menu |
| `PING` | Serial connection check; replies `OK PONG` |
| `INFO` | Curve, GPIO, serial and INA219 configuration |
| `STATUS` | Uptime, heap, CPU frequency, GPIO4 and button state |
| `FINAL` | One-shot test capture: exactly one ECDSA signing operation, GPIO4 active-LOW trigger, timestamped INA219 readouts before/during/after, signature verification, and capture-quality summary; no batch and no private-key dump |
| `SELFTEST` | Run cryptographic self-tests with external trigger suppressed |
| `TESTPR` | Sequential GPIO diagnostic; restores GPIO4 idle and GPIO27 button input afterward |
| `BUTTON` | Read button state on GPIO27 |
| `BUTTON_EVENT` | Read and clear the latched button press event |
| `INA219` | Read one voltage/current/power measurement |
| `BATCH INA219 <count>` | Read 1–1000 INA219 samples; `count` is required |
| `SIGN <message>` | SHA-256 a text message and sign/verify it |
| `SIGNHEX <64-hex digest>` | Sign/verify a 32-byte digest |
| `SIGNK <64-hex nonce> <message>` | Sign/verify message using a supplied test nonce |
| `SIGNHEXK <64-hex nonce> <64-hex digest>` | Sign/verify digest using a supplied test nonce |
| `SETK <64-hex nonce>` | Set in-RAM default test nonce; requires valid nonzero secp256k1 scalar |
| `SETD <64-hex private scalar>` | Set in-RAM test private scalar; requires valid nonzero secp256k1 scalar |
| `BATCH <count>` | Run 1–1000 test signatures with verification and GPIO4 read-backs |
| `BENCH <count>` | Benchmark 1–1000 signing operations |
| `PULSE [microseconds]` | One active-LOW GPIO4 pulse; range 1–1,000,000, default 1000 µs |
| `TRIGTEST [count]` | 1–12 repeated GPIO4 pulses; default 8, about 250 ms LOW and HIGH per cycle |
| `FULLIMP` | 160 randomized test signatures plus slow INA219 diagnostic readouts |
| `SCOPECAP` | 160 randomized test signatures and operation metadata for external capture |

`FINAL` is restricted to the compiled test key and performs exactly one signing operation. It does not export the nonce or private scalar. It reports raw INA219 readout timestamps and whether any I2C read overlapped the GPIO4 signing window. INA219 is a low-rate digital sensor read over I2C, so this one-shot trace does not claim private-key recovery.

Useful commands to try in sequence: `PING`, `INFO`, `STATUS`, `FINAL`, `INA219`,
`BATCH 35`, `BENCH 100`, `BATCH INA219 20`, `TRIGTEST 8`, `FULLIMP`.
`SCOPECAP` is for test-only external captures and emits `K_GT` labels; those
labels must never be included as CNN input features.


Firmware read-back passes are not a substitute for seeing the actual edge in
PulseView. For `BATCH 35`, if serial reports all 35 LOW/HIGH read-backs as PASS
but D0 still looks flat, capture timing/channel wiring/ground or the physical
GPIO4 connection must be checked; code cannot verify the external analyzer input.


## Serial Monitor menu (updated)

After reset/upload, the firmware waits about 1 second for the serial connection,
then prints a categorized ASCII command menu at 115200 baud. In Arduino IDE,
set the Serial Monitor line ending to **Both NL & CR**. The menu is also available
at any time by sending `HELP`, `MENU`, or `?` followed by Enter. Unknown commands
now point back to the menu.

Command keywords are case-insensitive (`ping`, `Ping`, and `PING` are accepted).
For `BATCH INA219 <count>`, the `INA219` subcommand is also case-insensitive.
Text passed to `SIGN <message>` is kept case-sensitive and is not converted to
uppercase.

The menu is grouped into basic diagnostics, ECDSA signing, batch/benchmark,
trigger experiments, and quick examples. See the menu output for exact argument
formats and count limits.


### INA219 measurement-quality update

The large FULLIMP sample buffer avoids storing derived values twice: the midpoint timestamp is computed from the stored I²C start/end timestamps, and calculated power is computed from voltage/current when reported. This retains the 2048-sample capacity and shunt readings while avoiding the extra static DRAM that caused the V3 linker overflow. Static contract checks cover this memory layout; verify compilation with the target ESP32 Arduino toolchain.

The INA219 driver now uses the Adafruit 32V/1A calibration profile (40 µA current LSB and 0.8 mW power-register LSB, assuming the common 0.1 Ω shunt). This fits the observed milliamp-level lab load better than the prior 32V/2A profile. The firmware preserves the sensor's power-register value (`P_MW`) and also reports `P_CALC_MW = bus_voltage_v * current_ma`, because the hardware power register is quantized.

`FINAL` discards two warm-up reads, reports a five-sample baseline mean and current spread, and emits measured sample interval min/max/mean. The sampler uses a periodic scheduler so the I²C transaction time is not added to every nominal period. These changes improve instrumentation diagnostics; they do not turn INA219 into a high-speed waveform ADC and do not implement private-key recovery.


### RAM fix and SBATCH regression guard

The diagnostic build does not keep a separate per-operation accumulator for calculated power; the summary cross-check derives it from average voltage and current, while each raw sample still emits `P_CALC_MW = V * I`. This avoids the extra static DRAM consumed by one double per operation. The `SBATCH` command is now a minimal GPIO4-only pulse generator for physical wiring/PulseView checks. It is intentionally not a signing batch. The original `BATCH` route remains separate and unchanged.

`FINAL` is still a one-operation instrumentation report. It does not recover a private key from INA219 measurements. No CNN or private-key extraction routine is included.


## RGB status LED (optional)

The 4-pin RGB LED is used only as a status indicator: blue = ready after boot, purple = FULLIMP running, green = FULLIMP passed, red = FULLIMP failed. The result color remains visible until the next FULLIMP run or restart. It uses GPIO25 (R), GPIO26 (G), and GPIO32 (B). Use one 220-330 ohm resistor in series with each color leg; do not use one resistor on the common leg.

The default configuration is common-cathode: connect the common leg to GND. If your LED is common-anode, connect its common leg to 3V3 and set `RGB_LED_COMMON_ANODE = true` in `src/core/config.h`. Never connect 3V3 or GND directly to a color leg without a series resistor. GPIO4 remains the Logic Analyzer trigger; GPIO27 remains the button; INA219 remains on GPIO21/22.
