from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ECDSA_DIR = Path(__file__).resolve().parents[1] / "ECDSA"
if str(ECDSA_DIR) not in sys.path:
    sys.path.insert(0, str(ECDSA_DIR))

from Lattice_key6 import FPYLLL_AVAILABLE, HNPConfig, HNPLatticeSolver
from dataset_generator3 import CURVE, generate_dataset


def evaluate_threshold(leaked_bits: int, sample_count: int, seed: int = 20260919) -> dict:
    public_samples, ground_truth = generate_dataset(CURVE, sample_count=sample_count, leaked_bits=leaked_bits, seed=seed)
    q = CURVE.n
    prefix_by_id = {sample.sample_id: sample.leaked_nonce_prefix for sample in ground_truth.samples}
    t_list: list[int] = []
    u_list: list[int] = []
    a_list: list[int] = []

    for record in public_samples:
        inverse_s = pow(int(record.s), -1, q)
        t_i = (inverse_s * int(record.r)) % q
        u_i = (inverse_s * int(record.z)) % q
        hidden_bits = q.bit_length() - leaked_bits
        a_i = prefix_by_id[int(record.sample_id)] << hidden_bits
        t_list.append(t_i)
        u_list.append(u_i)
        a_list.append(a_i)

    solver = HNPLatticeSolver(HNPConfig(leaked_bits=leaked_bits, num_samples=sample_count))
    reduction_error = None
    backend = "unavailable"
    try:
        recovered, backend = solver.solve(t_list, u_list, a_list)
    except RuntimeError as exc:
        recovered = None
        reduction_error = str(exc)
    relation_ok = solver.verify_expected_vector(ground_truth.private_key, t_list, u_list, a_list)
    return {
        "leaked_bits": leaked_bits,
        "reduction_backend": backend,
        "reduction_executed": reduction_error is None,
        "sample_count": sample_count,
        "relation_ok": relation_ok,
        "recovered_private_key": recovered,
        "matches_private_key": recovered == ground_truth.private_key,
        "reduction_error": reduction_error,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="Benchmark HNP recovery thresholds on perfect leakage.")
    parser.add_argument("--leaked-bits", nargs="*", type=int, default=[8, 12])
    parser.add_argument("--sample-counts", nargs="*", type=int, default=[20, 40])
    args = parser.parse_args()

    rows = []
    for leaked_bits in args.leaked_bits:
        for sample_count in args.sample_counts:
            rows.append(evaluate_threshold(leaked_bits, sample_count))

    print(json.dumps(rows, indent=2))


if __name__ == "__main__":
    main()
