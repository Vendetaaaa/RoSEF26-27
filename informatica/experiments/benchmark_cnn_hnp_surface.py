from __future__ import annotations

import argparse
import csv
import json
import random
import sys
import tempfile
import time
from pathlib import Path

import torch

ROOT = Path(__file__).resolve().parents[2]
ECDSA_DIR = ROOT / "informatica" / "ECDSA"
CNN_DIR = ROOT / "informatica" / "cnn"
for path in (ECDSA_DIR, CNN_DIR):
    if str(path) not in sys.path:
        sys.path.insert(0, str(path))

from Lattice_key6 import FPYLLL_AVAILABLE, HNPConfig, HNPLatticeSolver  # noqa: E402
from dataset_generator3 import CURVE, generate_dataset  # noqa: E402
from ecdsa_leakage_model4 import add_gaussian_noise, generate_clean_leakage  # noqa: E402
from train import fit_model  # noqa: E402
from utils import load_profile_dataset, split_dataset_by_ids  # noqa: E402

DEFAULT_OUTPUT = ROOT / "results" / "tables" / "cnn-hnp-parameter-sweep.csv"


def make_split(sample_count: int, seed: int) -> dict[str, list[int]]:
    ids = list(range(sample_count))
    random.Random(seed).shuffle(ids)
    train_end = int(sample_count * 0.60)
    val_end = train_end + int(sample_count * 0.15)
    return {"train": ids[:train_end], "validation": ids[train_end:val_end], "test": ids[val_end:]}


def make_profile(public, truth, sigma: float, seed: int, leaked_bits: int) -> list[dict]:
    rng = random.Random(seed)
    truth_by_id = {sample.sample_id: sample for sample in truth.samples}
    width = CURVE.n.bit_length()
    profile = []
    for record in public:
        nonce = truth_by_id[record.sample_id].nonce_k
        clean = generate_clean_leakage(nonce, width, mode="HW", alpha=1.0)
        noisy = add_gaussian_noise(clean, sigma, rng)
        bits = [(nonce >> position) & 1 for position in range(width - 1, width - leaked_bits - 1, -1)]
        profile.append(
            {
                "sample_id": record.sample_id,
                "z": record.z,
                "r": record.r,
                "s": record.s,
                "leaked_bits": leaked_bits,
                "trace": noisy,
                "bit_labels": bits,
            }
        )
    return profile


def prediction_prefix(bits: list[int]) -> int:
    prefix = 0
    for bit in bits:
        prefix = (prefix << 1) | int(bit)
    return prefix


def build_hnp_inputs(public_by_id: dict[int, dict], prefixes: dict[int, int], sample_ids: list[int], leaked_bits: int):
    q = CURVE.n
    hidden_bits = q.bit_length() - leaked_bits
    t_list: list[int] = []
    u_list: list[int] = []
    a_list: list[int] = []
    for sample_id in sample_ids:
        record = public_by_id[sample_id]
        inv_s = pow(int(record["s"]), -1, q)
        t_list.append(inv_s * int(record["r"]) % q)
        u_list.append(inv_s * int(record["z"]) % q)
        a_list.append(int(prefixes[sample_id]) << hidden_bits)
    return t_list, u_list, a_list


def solve_hnp(
    public_by_id: dict[int, dict],
    prefixes: dict[int, int],
    sample_ids: list[int],
    leaked_bits: int,
    private_key: int,
    *,
    require_fpylll: bool,
) -> dict:
    t_list, u_list, a_list = build_hnp_inputs(public_by_id, prefixes, sample_ids, leaked_bits)
    solver = HNPLatticeSolver(HNPConfig(leaked_bits=leaked_bits, num_samples=len(sample_ids)))
    try:
        relation_ok = solver.verify_expected_vector(private_key, t_list, u_list, a_list)
    except ValueError:
        relation_ok = False
    if not relation_ok:
        return {
            "relation_ok": False,
            "key_recovered": False,
            "backend": None,
            "runtime_seconds": None,
            "error": "expected HNP vector failed embedding validation",
        }

    start = time.perf_counter()
    try:
        recovered, backend = solver.solve(
            t_list, u_list, a_list, require_fpylll=require_fpylll
        )
        elapsed = time.perf_counter() - start
    except RuntimeError as exc:
        return {
            "relation_ok": True,
            "key_recovered": False,
            "backend": "unavailable",
            "runtime_seconds": time.perf_counter() - start,
            "error": str(exc),
        }
    return {
        "relation_ok": True,
        "key_recovered": recovered == private_key,
        "backend": backend,
        "runtime_seconds": elapsed,
        "error": None,
    }


def run_surface(
    leaked_bits_values: list[int],
    sigma_values: list[float],
    sample_counts: list[int],
    *,
    sample_count: int,
    epochs: int,
    seed: int,
    require_fpylll: bool,
    output_path: Path,
) -> list[dict]:
    if require_fpylll and not FPYLLL_AVAILABLE:
        raise RuntimeError("fpylll is required for the reference parameter sweep but is not installed.")
    if max(sample_counts) > 40:
        raise ValueError("The shared CNN/HNP attack split contains exactly 40 test samples.")

    output_rows: list[dict] = []
    for leaked_bits in leaked_bits_values:
        public, truth = generate_dataset(CURVE, sample_count=sample_count, leaked_bits=leaked_bits, seed=seed)
        public_dict = [
            {
                "sample_id": x.sample_id,
                "z": x.z,
                "r": x.r,
                "s": x.s,
                "leaked_bits": x.leaked_bits,
            }
            for x in public
        ]
        public_by_id = {int(x["sample_id"]): x for x in public_dict}
        oracle_prefixes = {sample.sample_id: sample.leaked_nonce_prefix for sample in truth.samples}
        split = make_split(sample_count, seed + 1)
        if len(split["test"]) != 40:
            raise RuntimeError("Benchmark split must contain 40 attack samples.")

        for sigma in sigma_values:
            profile = make_profile(public, truth, sigma, seed, leaked_bits)
            with tempfile.TemporaryDirectory(prefix="rosef_cnn_surface_") as temp_dir:
                profile_path = Path(temp_dir) / "profile.json"
                split_path = Path(temp_dir) / "split.json"
                profile_path.write_text(json.dumps(profile), encoding="utf-8")
                split_path.write_text(json.dumps(split), encoding="utf-8")
                traces, labels, records = load_profile_dataset(profile_path)

                config = {
                    "dataset": {"sequence_length": leaked_bits},
                    "model": {"in_channels": 2, "out_channels": 8, "kernel_size": 3, "num_classes": 2},
                    "training": {
                        "batch_size": 32,
                        "epochs": epochs,
                        "learning_rate": 0.005,
                        "weight_decay": 0.0001,
                        "random_seed": seed + leaked_bits,
                    },
                }
                traces = traces[:, :, :leaked_bits]
                labels = labels[:, :leaked_bits]
                split_data = split_dataset_by_ids(traces, labels, records, split_path)
                train_x, train_y, test_x, test_y = split_data[0], split_data[1], split_data[4], split_data[5]
                model, metrics = fit_model(train_x, train_y, split_data[2], split_data[3], config)

                with torch.no_grad():
                    test_predictions = model(test_x).argmax(dim=1)
                test_bit_accuracy = float((test_predictions == test_y).float().mean().item())
                test_prefix_accuracy = float((test_predictions == test_y).all(dim=1).float().mean().item())
                predicted_by_id = {
                    int(sample_id): prediction_prefix(bits)
                    for sample_id, bits in zip(split["test"], test_predictions.cpu().tolist())
                }
                exact_prefixes = int(round(test_prefix_accuracy * len(split["test"])))

                for m in sample_counts:
                    sample_ids = split["test"][:m]
                    oracle_result = solve_hnp(
                        public_by_id,
                        oracle_prefixes,
                        sample_ids,
                        leaked_bits,
                        truth.private_key,
                        require_fpylll=require_fpylll,
                    )
                    cnn_result = solve_hnp(
                        public_by_id,
                        predicted_by_id,
                        sample_ids,
                        leaked_bits,
                        truth.private_key,
                        require_fpylll=require_fpylll,
                    )
                    output_rows.append(
                        {
                            "leaked_bits": leaked_bits,
                            "sigma": sigma,
                            "m": m,
                            "seed": seed,
                            "epochs": epochs,
                            "cnn_test_bit_accuracy": test_bit_accuracy,
                            "cnn_test_prefix_accuracy": test_prefix_accuracy,
                            "cnn_test_exact_prefixes": exact_prefixes,
                            "oracle_hnp_key_recovered": oracle_result["key_recovered"],
                            "cnn_hnp_key_recovered": cnn_result["key_recovered"],
                            "hnp_backend": cnn_result["backend"],
                            "oracle_hnp_runtime_seconds": oracle_result["runtime_seconds"],
                            "cnn_hnp_runtime_seconds": cnn_result["runtime_seconds"],
                            "oracle_hnp_error": oracle_result["error"],
                            "cnn_hnp_error": cnn_result["error"],
                        }
                    )

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(output_rows[0]))
        writer.writeheader()
        writer.writerows(output_rows)
    return output_rows



def run_multi_seed_surface(
    leaked_bits_values: list[int],
    sigma_values: list[float],
    sample_counts: list[int],
    *,
    sample_count: int,
    epochs: int,
    seeds: list[int],
    require_fpylll: bool,
    output_path: Path,
) -> tuple[list[dict], list[dict]]:
    all_rows: list[dict] = []
    for seed in seeds:
        all_rows.extend(
            run_surface(
                leaked_bits_values,
                sigma_values,
                sample_counts,
                sample_count=sample_count,
                epochs=epochs,
                seed=seed,
                require_fpylll=require_fpylll,
                output_path=Path("/tmp") / f"rosef-seed-{seed}.csv",
            )
        )

    grouped: dict[tuple[int, float, int], list[dict]] = {}
    for row in all_rows:
        grouped.setdefault((int(row["leaked_bits"]), float(row["sigma"]), int(row["m"])), []).append(row)

    summary: list[dict] = []
    for (leaked_bits, sigma, m), rows in sorted(grouped.items()):
        def mean(key: str) -> float | None:
            values = [float(row[key]) for row in rows if row[key] is not None and row[key] != ""]
            return sum(values) / len(values) if values else None

        cnn_successes = sum(bool(row["cnn_hnp_key_recovered"]) for row in rows)
        oracle_successes = sum(bool(row["oracle_hnp_key_recovered"]) for row in rows)
        summary.append(
            {
                "leaked_bits": leaked_bits,
                "sigma": sigma,
                "m": m,
                "runs": len(rows),
                "cnn_hnp_recoveries": cnn_successes,
                "cnn_hnp_success_rate": cnn_successes / len(rows),
                "oracle_hnp_recoveries": oracle_successes,
                "oracle_hnp_success_rate": oracle_successes / len(rows),
                "mean_cnn_test_bit_accuracy": mean("cnn_test_bit_accuracy"),
                "mean_cnn_test_prefix_accuracy": mean("cnn_test_prefix_accuracy"),
                "mean_cnn_hnp_runtime_seconds": mean("cnn_hnp_runtime_seconds"),
                "mean_oracle_hnp_runtime_seconds": mean("oracle_hnp_runtime_seconds"),
            }
        )

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(all_rows[0]))
        writer.writeheader()
        writer.writerows(all_rows)

    summary_path = output_path.with_name(output_path.stem + "-summary.csv")
    with summary_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(summary[0]))
        writer.writeheader()
        writer.writerows(summary)
    return all_rows, summary


def main() -> int:
    parser = argparse.ArgumentParser(description="Quantify CNN accuracy and HNP recovery over (ell, sigma, m).")
    parser.add_argument("--leaked-bits", nargs="*", type=int, default=[8, 12])
    parser.add_argument("--sigmas", nargs="*", type=float, default=[0.10, 0.15, 0.20, 0.25])
    parser.add_argument("--sample-counts", nargs="*", type=int, default=[20, 40])
    parser.add_argument("--dataset-samples", type=int, default=160)
    parser.add_argument("--epochs", type=int, default=100)
    parser.add_argument("--seed", type=int, default=20260919, help="Compatibility option for a single seed.")
    parser.add_argument("--seeds", nargs="*", type=int, default=None)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument(
        "--allow-sympy", action="store_true",
        help="Allow the slow SymPy fallback. Reference results should use fpylll instead.",
    )
    args = parser.parse_args()
    seeds = args.seeds if args.seeds else [args.seed]
    _, summary = run_multi_seed_surface(
        args.leaked_bits,
        args.sigmas,
        args.sample_counts,
        sample_count=args.dataset_samples,
        epochs=args.epochs,
        seeds=seeds,
        require_fpylll=not args.allow_sympy,
        output_path=args.output,
    )
    summary_path = args.output.with_name(args.output.stem + "-summary.csv")
    print(f"[PASS] Parameter sweep written to {args.output}.")
    print(f"[PASS] Aggregated success rates written to {summary_path}.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
