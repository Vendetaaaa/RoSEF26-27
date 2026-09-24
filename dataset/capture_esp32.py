from __future__ import annotations

import argparse
import json
import time
from pathlib import Path
from typing import TextIO

try:
    import serial
except ImportError:  # pragma: no cover
    serial = None

ROOT = Path(__file__).resolve().parents[2]
ARTIFACTS = ROOT / "informatica" / "artifacts"


def _require_serial() -> None:
    if serial is None:
        raise RuntimeError("pyserial is required for ESP32 capture: pip install pyserial")


def _read_until_ok(port: TextIO, timeout_s: float = 5.0) -> list[str]:
    deadline = time.monotonic() + timeout_s
    lines: list[str] = []
    while time.monotonic() < deadline:
        raw = port.readline()
        if not raw:
            continue
        line = raw.decode("utf-8", errors="replace").strip()
        if line:
            lines.append(line)
        if line == "OK":
            return lines
        if line.startswith("ERR"):
            raise RuntimeError(line)
    raise TimeoutError("ESP32 did not return a response before timeout.")


def capture_signatures(
    port_name: str,
    *,
    count: int,
    baud: int = 115200,
    output: Path = ARTIFACTS / "esp32_signatures.jsonl",
) -> None:
    _require_serial()
    if count <= 0:
        raise ValueError("count must be positive")

    output.parent.mkdir(parents=True, exist_ok=True)
    records = []
    with serial.Serial(port_name, baudrate=baud, timeout=0.2) as port, output.open("w", encoding="utf-8") as handle:
        time.sleep(1.0)
        port.write(b"PING\n")
        _read_until_ok(port)

        for sample_id in range(count):
            message = f"RoSEF trace {sample_id:06d}"
            port.write(f"SIGN {message}\n".encode("utf-8"))
            lines = _read_until_ok(port)
            response = {"sample_id": sample_id, "message": message, "raw_response": lines}
            for line in lines:
                if line.startswith("R="):
                    response["r"] = line[2:]
                elif line.startswith("S="):
                    response["s"] = line[2:]
            handle.write(json.dumps(response) + "\n")
            records.append(response)

    print(f"[PASS] Captured {len(records)} ESP32 signatures to {output}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Capture ECDSA signature metadata from the ESP32 serial protocol.")
    parser.add_argument("port")
    parser.add_argument("--count", type=int, default=100)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--output", type=Path, default=ARTIFACTS / "esp32_signatures.jsonl")
    args = parser.parse_args()
    capture_signatures(args.port, count=args.count, baud=args.baud, output=args.output)


if __name__ == "__main__":
    main()
