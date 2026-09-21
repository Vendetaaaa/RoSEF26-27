from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SIM_DIR = ROOT / "informatica" / "simulator"
ECDSA_DIR = ROOT / "informatica" / "ECDSA"
for path in (SIM_DIR, ECDSA_DIR):
    if str(path) not in sys.path:
        sys.path.insert(0, str(path))

from config import SimulationConfig
from generator import generate_simulated_dataset
from noise import normalize


def test_simulation_is_reproducible() -> None:
    config = SimulationConfig(leaked_bits=12, trace_length=12, noise_sigma=0.15, seed=99)
    first = generate_simulated_dataset(sample_count=6, config=config)[2]
    second = generate_simulated_dataset(sample_count=6, config=config)[2]
    assert first == second
    assert len(first) == 6
    assert len(first[0].trace) == 12
    assert len(first[0].bit_labels) == 12


def test_simulation_rejects_invalid_configuration() -> None:
    config = SimulationConfig(leaked_bits=13, trace_length=12)
    try:
        config.validate()
    except ValueError as exc:
        assert "trace_length" in str(exc)
    else:
        raise AssertionError("Invalid simulator configuration was accepted.")


def test_normalize_constant_signal() -> None:
    assert normalize([3.0, 3.0, 3.0]) == [0.0, 0.0, 0.0]
