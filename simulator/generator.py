from __future__ import annotations

import random
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parents[1]
ECDSA_DIR = ROOT / "ECDSA"
if str(ECDSA_DIR) not in sys.path:
    sys.path.insert(0, str(ECDSA_DIR))

from dataset_generator3 import CURVE, GroundTruth, PublicSample, generate_dataset  # noqa: E402
from ecdsa_leakage_model4 import generate_clean_leakage, integer_to_bits  # noqa: E402
from config import SimulationConfig
from noise import add_gaussian_noise, normalize


@dataclass(frozen=True)
class SimulatedTrace:
    sample_id: int
    trace: tuple[float, ...]
    bit_labels: tuple[int, ...]


def simulate_traces(
    public_samples: Iterable[PublicSample],
    ground_truth: GroundTruth,
    config: SimulationConfig,
) -> list[SimulatedTrace]:
    config.validate()
    rng = random.Random(config.seed)
    truth = {sample.sample_id: sample for sample in ground_truth.samples}
    result: list[SimulatedTrace] = []
    width = CURVE.n.bit_length()

    for public in public_samples:
        if public.sample_id not in truth:
            raise KeyError(f"Missing ground-truth sample {public.sample_id}.")
        nonce = truth[public.sample_id].nonce_k
        clean = generate_clean_leakage(nonce, width, config.leakage_model, config.alpha)
        noisy = add_gaussian_noise(clean[: config.trace_length], config.noise_sigma, rng)
        if config.normalize_trace:
            noisy = normalize(noisy)
        labels = integer_to_bits(nonce, width)[: config.leaked_bits]
        result.append(SimulatedTrace(public.sample_id, tuple(noisy), tuple(labels)))
    return result


def generate_simulated_dataset(
    sample_count: int = 160,
    config: SimulationConfig | None = None,
) -> tuple[list[PublicSample], GroundTruth, list[SimulatedTrace]]:
    config = config or SimulationConfig()
    public, truth = generate_dataset(
        CURVE,
        sample_count=sample_count,
        leaked_bits=config.leaked_bits,
        seed=config.seed,
    )
    traces = simulate_traces(public, truth, config)
    return public, truth, traces
