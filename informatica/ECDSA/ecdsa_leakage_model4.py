from __future__ import annotations

import json
import math
import random
import statistics
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Tuple

from dataset_generator3 import CURVE, load_ground_truth, load_public_dataset

ARTIFACTS_DIR = Path(__file__).resolve().parents[1] / "artifacts"
PROFILE_DATASET_PATH = ARTIFACTS_DIR / "profile_dataset.json"

LEAKAGE_MODE = "HW"
LEAKAGE_ALPHA = 1.0
NOISE_SIGMA = 0.50
PROFILE_NOISE_SIGMA = 0.15
RANDOM_SEED = 20260919


def hamming_weight(value: int) -> int:
    if value < 0:
        raise ValueError("hamming_weight expects a non-negative integer.")
    return value.bit_count()


def hamming_distance(old_value: int, new_value: int) -> int:
    if old_value < 0 or new_value < 0:
        raise ValueError("hamming_distance expects non-negative integers.")
    return (old_value ^ new_value).bit_count()


def integer_to_bits(value: int, width: int) -> List[int]:
    return [(value >> position) & 1 for position in range(width - 1, -1, -1)]


def generate_clean_leakage(nonce_k: int, width: int, mode: str = LEAKAGE_MODE, alpha: float = LEAKAGE_ALPHA) -> List[float]:
    mode = mode.upper()
    if mode not in {"HW", "HD"}:
        raise ValueError("mode must be HW or HD.")
    bits = integer_to_bits(nonce_k, width)
    trace: List[float] = []
    prefix = 0
    previous_prefix = 0
    for bit in bits:
        prefix = (prefix << 1) | bit
        if mode == "HW":
            leakage = alpha * hamming_weight(prefix)
        else:
            leakage = alpha * hamming_distance(previous_prefix, prefix)
        trace.append(float(leakage))
        previous_prefix = prefix
    return trace


def add_gaussian_noise(clean_trace: List[float], sigma: float, rng: random.Random) -> List[float]:
    if sigma < 0:
        raise ValueError("sigma must be non-negative.")
    return [value + rng.gauss(0.0, sigma) for value in clean_trace]


def estimate_snr_db(clean_trace: List[float], sigma: float) -> float:
    signal_power = statistics.pvariance(clean_trace) if len(clean_trace) > 1 else 0.0
    noise_power = sigma * sigma
    if noise_power == 0:
        return float("inf")
    if signal_power == 0:
        return float("-inf")
    return 10.0 * math.log10(signal_power / noise_power)


def classify_bits(noisy_trace: List[float], width: int, mode: str = LEAKAGE_MODE, alpha: float = LEAKAGE_ALPHA) -> List[Tuple[int, float]]:
    mode = mode.upper()
    guessed_prefix = 0
    previous_prefix = 0
    results: List[Tuple[int, float]] = []

    for observed_sample in noisy_trace[:width]:
        predictions = {}
        for candidate_bit in (0, 1):
            candidate_prefix = (guessed_prefix << 1) | candidate_bit
            if mode == "HW":
                prediction = alpha * hamming_weight(candidate_prefix)
            else:
                prediction = alpha * hamming_distance(previous_prefix, candidate_prefix)
            predictions[candidate_bit] = prediction

        d0 = abs(observed_sample - predictions[0])
        d1 = abs(observed_sample - predictions[1])
        guessed_bit = 0 if d0 <= d1 else 1
        separation = abs(predictions[0] - predictions[1])
        confidence = 0.0 if separation == 0 else min(1.0, abs(d0 - d1) / separation)
        results.append((guessed_bit, confidence))
        guessed_prefix = (guessed_prefix << 1) | guessed_bit
        previous_prefix = guessed_prefix

    return results


def ensure_dataset() -> tuple[list[dict], dict]:
    public_path = ARTIFACTS_DIR / "public_dataset.json"
    oracle_path = ARTIFACTS_DIR / "oracle_dataset.json"
    if not public_path.exists() or not oracle_path.exists():
        raise FileNotFoundError("Run dataset_generator3.py before ecdsa_leakage_model4.py.")
    return load_public_dataset(public_path), load_ground_truth(oracle_path)


def build_profile_dataset(sigma: float = PROFILE_NOISE_SIGMA, mode: str = LEAKAGE_MODE, seed: int = RANDOM_SEED) -> list[dict]:
    public_records, oracle = ensure_dataset()
    rng = random.Random(seed)
    width = CURVE.n.bit_length()
    profile_records: list[dict] = []
    truth_samples = oracle["samples"]

    for record in public_records:
        truth = truth_samples[str(record["sample_id"])]
        nonce_k = int(truth["nonce_k"])
        clean = generate_clean_leakage(nonce_k, width, mode, LEAKAGE_ALPHA)
        noisy = add_gaussian_noise(clean, sigma, rng)
        bits = integer_to_bits(nonce_k, width)
        profile_records.append(
            {
                "sample_id": record["sample_id"],
                "z": record["z"],
                "r": record["r"],
                "s": record["s"],
                "leaked_bits": record["leaked_bits"],
                "trace": noisy,
                "bit_labels": bits,
            }
        )

    ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)
    PROFILE_DATASET_PATH.write_text(json.dumps(profile_records), encoding="utf-8")
    return profile_records


def noise_experiment() -> None:
    public_records, oracle = ensure_dataset()
    rng = random.Random(RANDOM_SEED)
    width = CURVE.n.bit_length()
    nonce_values = [int(oracle["samples"][str(record["sample_id"])]["nonce_k"]) for record in public_records[:100]]
    print("NOISE / DIRECT DECODER EXPERIMENT")
    print(f"{'Sigma':>8} | {'Avg SNR (dB)':>14} | {'Bit Accuracy':>14}")
    print("-" * 46)
    for sigma in (0.25, 0.50, 1.00, 1.50, 2.00, 3.00):
        correct = 0
        total = 0
        snr_values = []
        for nonce_k in nonce_values:
            clean = generate_clean_leakage(nonce_k, width)
            noisy = add_gaussian_noise(clean, sigma, rng)
            guesses = classify_bits(noisy, width)
            predicted = [bit for bit, _ in guesses]
            actual = integer_to_bits(nonce_k, width)
            correct += sum(a == b for a, b in zip(actual, predicted))
            total += width
            snr_values.append(estimate_snr_db(clean, sigma))
        accuracy = 100.0 * correct / total
        print(f"{sigma:8.2f} | {statistics.fmean(snr_values):14.2f} | {accuracy:13.2f}%")


def main() -> None:
    profile_records = build_profile_dataset()
    print(f"[PASS] Generated {len(profile_records)} traces from the shared ECDSA dataset.")
    print(f"[PASS] Trace width: {CURVE.n.bit_length()} bits.")
    print(f"[PASS] Profile dataset written to {PROFILE_DATASET_PATH}.")
    noise_experiment()
    print("[PASS] Leakage stage complete.")


if __name__ == "__main__":
    main()
