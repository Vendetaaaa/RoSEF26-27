from __future__ import annotations

import argparse
import csv
import json
import statistics
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ECDSA_DIR = ROOT / "informatica" / "ECDSA"
SIM_DIR = ROOT / "informatica" / "simulator"
for path in (ECDSA_DIR, SIM_DIR):
    if str(path) not in sys.path:
        sys.path.insert(0, str(path))

from dataset_generator3 import CURVE, generate_dataset  # noqa: E402
from ecdsa_leakage_model4 import classify_bits, estimate_snr_db, generate_clean_leakage, add_gaussian_noise  # noqa: E402
from Lattice_key6 import HNPConfig, HNPLatticeSolver  # noqa: E402

OUTPUT_DIR = ROOT / "results" / "raw"
CSV_PATH = ROOT / "results" / "tables" / "final-results.csv"


def direct_decoder_experiment(sample_count: int, leaked_bits: int, sigma: float, seed: int) -> dict:
    public, truth = generate_dataset(CURVE, sample_count=sample_count, leaked_bits=leaked_bits, seed=seed)
    truth_by_id = {sample.sample_id: sample for sample in truth.samples}
    import random
    rng = random.Random(seed + 1)
    width = CURVE.n.bit_length()
    correct = 0
    total = 0
    prefix_correct = 0
    snr_values: list[float] = []
    for record in public:
        nonce = truth_by_id[record.sample_id].nonce_k
        clean = generate_clean_leakage(nonce, width)
        noisy = add_gaussian_noise(clean, sigma, rng)
        predicted = [bit for bit, _ in classify_bits(noisy, width)][:leaked_bits]
        actual = [int(bit) for bit in f"{nonce:0{width}b}"[:leaked_bits]]
        correct += sum(a == b for a, b in zip(actual, predicted))
        total += leaked_bits
        prefix_correct += int(predicted == actual)
        snr_values.append(estimate_snr_db(clean, sigma))
    return {
        "experiment": "direct_decoder",
        "seed": seed,
        "samples": sample_count,
        "leaked_bits": leaked_bits,
        "sigma": sigma,
        "bit_accuracy": correct / total,
        "prefix_accuracy": prefix_correct / sample_count,
        "key_recovered": False,
        "relation_valid": None,
        "runtime_seconds": None,
        "avg_snr_db": statistics.fmean(snr_values),
    }


def hnp_relation_experiment(sample_count: int, leaked_bits: int, seed: int) -> dict:
    public, truth = generate_dataset(CURVE, sample_count=sample_count, leaked_bits=leaked_bits, seed=seed)
    truth_by_id = {sample.sample_id: sample for sample in truth.samples}
    q = CURVE.n
    hidden_bits = q.bit_length() - leaked_bits
    t_list: list[int] = []
    u_list: list[int] = []
    a_list: list[int] = []
    for record in public:
        s_inv = pow(record.s, -1, q)
        t_list.append(s_inv * record.r % q)
        u_list.append(s_inv * record.z % q)
        a_list.append(truth_by_id[record.sample_id].leaked_nonce_prefix << hidden_bits)
    solver = HNPLatticeSolver(HNPConfig(leaked_bits=leaked_bits, num_samples=sample_count))
    start = time.perf_counter()
    relation_ok = solver.verify_expected_vector(truth.private_key, t_list, u_list, a_list)
    elapsed = time.perf_counter() - start
    return {
        "experiment": "hnp_relation",
        "seed": seed,
        "samples": sample_count,
        "leaked_bits": leaked_bits,
        "sigma": 0.0,
        "bit_accuracy": 1.0,
        "prefix_accuracy": 1.0,
        "key_recovered": False,
        "relation_valid": relation_ok,
        "runtime_seconds": elapsed,
        "avg_snr_db": None,
    }


def write_results(rows: list[dict]) -> None:
    CSV_PATH.parent.mkdir(parents=True, exist_ok=True)
    fieldnames = [
        "experiment", "seed", "samples", "leaked_bits", "sigma",
        "bit_accuracy", "prefix_accuracy", "key_recovered", "relation_valid",
        "runtime_seconds", "avg_snr_db",
    ]
    with CSV_PATH.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    (OUTPUT_DIR / "latest-results.json").write_text(json.dumps(rows, indent=2), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Run reproducible synthetic ECDSA/leakage experiments.")
    parser.add_argument("--samples", type=int, default=40)
    parser.add_argument("--leaked-bits", type=int, default=12)
    parser.add_argument("--sigma", type=float, default=0.15)
    parser.add_argument("--seed", type=int, default=20260919)
    parser.add_argument("--skip-hnp-relation", action="store_true")
    args = parser.parse_args()
    if args.samples <= 0:
        parser.error("--samples must be positive")
    rows = [direct_decoder_experiment(args.samples, args.leaked_bits, args.sigma, args.seed)]
    if not args.skip_hnp_relation:
        rows.append(hnp_relation_experiment(args.samples, args.leaked_bits, args.seed))
    write_results(rows)
    print(json.dumps(rows, indent=2))
    print(f"[PASS] Results written to {CSV_PATH}")


if __name__ == "__main__":
    main()
