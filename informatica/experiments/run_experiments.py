from __future__ import annotations

import argparse
import csv
import json
import random
import statistics
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ECDSA_DIR = ROOT / "informatica" / "ECDSA"
SIM_DIR = ROOT / "informatica" / "simulator"
LATTICE_DIR = ROOT / "informatica" / "lattice"
for path in (ECDSA_DIR, SIM_DIR, LATTICE_DIR):
    if str(path) not in sys.path:
        sys.path.insert(0, str(path))

from dataset_generator3 import CURVE, generate_dataset  # noqa: E402
from ecdsa_leakage_model4 import classify_bits, estimate_snr_db, generate_clean_leakage, add_gaussian_noise  # noqa: E402
from solver import HNPConfig, HNPLatticeSolver, build_hnp_inputs  # noqa: E402

OUTPUT_DIR = ROOT / "results" / "raw"
CSV_PATH = ROOT / "results" / "tables" / "final-results.csv"


def direct_decoder_experiment(sample_count: int, leaked_bits: int, sigma: float, seed: int) -> dict:
    public, truth = generate_dataset(CURVE, sample_count=sample_count, leaked_bits=leaked_bits, seed=seed)
    truth_by_id = {sample.sample_id: sample for sample in truth.samples}
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
    prefixes = {sample.sample_id: sample.leaked_nonce_prefix for sample in truth.samples}
    t_list, u_list, a_list = build_hnp_inputs(public, prefixes, CURVE.n, leaked_bits)
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


def hnp_recovery_experiment(sample_count: int, leaked_bits: int, seed: int, run_lll: bool = True) -> dict:
    public, truth = generate_dataset(CURVE, sample_count=sample_count, leaked_bits=leaked_bits, seed=seed)
    prefixes = {sample.sample_id: sample.leaked_nonce_prefix for sample in truth.samples}
    t_list, u_list, a_list = build_hnp_inputs(public, prefixes, CURVE.n, leaked_bits)
    solver = HNPLatticeSolver(HNPConfig(leaked_bits=leaked_bits, num_samples=sample_count))
    relation_ok = solver.verify_expected_vector(truth.private_key, t_list, u_list, a_list)
    if not run_lll:
        return {
            "experiment": "hnp_recovery",
            "seed": seed, "samples": sample_count, "leaked_bits": leaked_bits,
            "sigma": 0.0, "bit_accuracy": 1.0, "prefix_accuracy": 1.0,
            "key_recovered": False, "relation_valid": relation_ok,
            "runtime_seconds": 0.0, "avg_snr_db": None, "backend": "skipped",
        }
    start = time.perf_counter()
    error = None
    recovered = None
    try:
        recovered = solver.solve(t_list, u_list, a_list)
    except RuntimeError as exc:
        error = str(exc)
    elapsed = time.perf_counter() - start
    return {
        "experiment": "hnp_recovery",
        "seed": seed, "samples": sample_count, "leaked_bits": leaked_bits,
        "sigma": 0.0, "bit_accuracy": 1.0, "prefix_accuracy": 1.0,
        "key_recovered": recovered == truth.private_key,
        "relation_valid": relation_ok, "runtime_seconds": elapsed,
        "avg_snr_db": None,
        "backend": "fpylll" if getattr(__import__("Lattice_key6"), "FPYLLL_AVAILABLE", False) else "sympy",
        "reduction_error": error,
    }


def write_results(rows: list[dict], append: bool = False) -> None:
    CSV_PATH.parent.mkdir(parents=True, exist_ok=True)
    existing: list[dict] = []
    if append and CSV_PATH.exists():
        with CSV_PATH.open(newline="", encoding="utf-8") as handle:
            existing = list(csv.DictReader(handle))
    all_rows = existing + rows
    fieldnames = sorted({key for row in all_rows for key in row})
    with CSV_PATH.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(all_rows)
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    (OUTPUT_DIR / "latest-results.json").write_text(json.dumps(rows, indent=2), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Run reproducible ECDSA leakage/HNP experiments.")
    parser.add_argument("--samples", type=int, default=40)
    parser.add_argument("--leaked-bits", type=int, default=12)
    parser.add_argument("--sigma", type=float, default=0.15)
    parser.add_argument("--seed", type=int, default=20260919)
    parser.add_argument("--skip-hnp-relation", action="store_true")
    parser.add_argument("--run-lll", action="store_true", help="Also attempt lattice reduction; may be expensive.")
    parser.add_argument("--append", action="store_true")
    args = parser.parse_args()
    if args.samples < 2:
        parser.error("--samples must be at least 2")
    if not 1 <= args.leaked_bits < CURVE.n.bit_length():
        parser.error("--leaked-bits must be between 1 and 255")
    if args.sigma < 0:
        parser.error("--sigma must be non-negative")
    rows = [direct_decoder_experiment(args.samples, args.leaked_bits, args.sigma, args.seed)]
    if not args.skip_hnp_relation:
        rows.append(hnp_relation_experiment(args.samples, args.leaked_bits, args.seed))
    if args.run_lll:
        rows.append(hnp_recovery_experiment(args.samples, args.leaked_bits, args.seed, run_lll=True))
    write_results(rows, append=args.append)
    print(json.dumps(rows, indent=2))
    print(f"[PASS] Results written to {CSV_PATH}")


if __name__ == "__main__":
    main()
