from __future__ import annotations

import argparse
import json
import time
from pathlib import Path

try:
    import serial
except ImportError:  # pragma: no cover
    serial = None

ROOT = Path(__file__).resolve().parents[2]
ARTIFACTS = ROOT / "informatica" / "artifacts"


def _require_serial() -> None:
    if serial is None:
        raise RuntimeError("pyserial is required for ESP32 capture: pip install pyserial")


def _read_batch(port, timeout_s: float = 240.0) -> list[str]:
    deadline = time.monotonic() + timeout_s
    lines: list[str] = []
    while time.monotonic() < deadline:
        raw = port.readline()
        if not raw:
            continue
        line = raw.decode("utf-8", errors="replace").strip()
        if not line:
            continue
        lines.append(line)
        if line == "OK SCOPECAP=PASS":
            return lines
        if line.startswith("ERR SCOPECAP=") or line.startswith("ERR "):
            raise RuntimeError("ESP32 capture failed: " + line)
    raise TimeoutError(
    )


def capture_signatures(
    port_name: str,
    *,
    baud: int = 115200,
    output: Path = ARTIFACTS / "esp32_scopecap_batches.jsonl",
    timeout_s: float = 240.0,
) -> None:
    _require_serial()
    output.parent.mkdir(parents=True, exist_ok=True)
    with serial.Serial(port_name, baudrate=baud, timeout=0.25) as port:
        time.sleep(1.0)
        port.reset_input_buffer()
        port.write(b"PING\n")
        port.flush()
        ping_lines = []
        deadline = time.monotonic() + 5.0
        while time.monotonic() < deadline:
            raw = port.readline()
            if raw:
                line = raw.decode("utf-8", errors="replace").strip()
                ping_lines.append(line)
                if line == "OK":
                    break
        else:
            raise TimeoutError("ESP32 did not respond to PING.")

        input("Arm the oscilloscope/DAQ on GPIO4 falling edge, then press Enter to start SCOPECAP...")
        port.write(b"SCOPECAP\n")
        port.flush()
        lines = _read_batch(port, timeout_s=timeout_s)

    record = {
        "capture_protocol": "SCOPECAP",
        "firmware_batch_size": 160,
        "serial_port": port_name,
        "baud": baud,
        "trigger": "GPIO4 falling edge (active-low)",
        "waveform_included": False,
        "waveform_note": "Capture analog traces separately with external oscilloscope/DAQ; this file contains serial metadata only.",
        "ping_response": ping_lines,
        "raw_response": lines,
        "success": True,
    }
    with output.open("w", encoding="utf-8") as handle:
        handle.write(json.dumps(record, ensure_ascii=False) + "\n")
    print(f"[PASS] SCOPECAP batch metadata saved to {output}")
    print("[NOTE] This is not a waveform file. Join external traces using operation metadata and trigger timing.")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Capture ESP32 SCOPECAP batch metadata for external oscilloscope/DAQ traces."
    )
    parser.add_argument("port", help="Serial port, e.g. COM5 or /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=240.0)
    parser.add_argument("--output", type=Path, default=ARTIFACTS / "esp32_scopecap_batches.jsonl")
    args = parser.parse_args()
    capture_signatures(args.port, baud=args.baud, output=args.output, timeout_s=args.timeout)


if __name__ == "__main__":
    main()
