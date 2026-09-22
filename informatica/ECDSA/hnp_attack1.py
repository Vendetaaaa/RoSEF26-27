from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Optional

from Lattice_key6 import FPYLLL_AVAILABLE, HNPConfig, HNPLatticeSolver
from dataset_generator3 import CURVE
from hnp_utils import compute_hnp_terms

ARTIFACTS_DIR = Path(__file__).resolve().parents[1] / "artifacts"
PUBLIC_DATASET_PATH = ARTIFACTS_DIR / "public_dataset.json"
CNN_PREDICTIONS_PATH = ARTIFACTS_DIR / "cnn_predictions.json"
HNP_RESULT_PATH = ARTIFACTS_DIR / "hnp_result.json"


def load_prefixes(source: str) -> dict[int, int]:
    if source == "cnn":
        payload = json.loads(CNN_PREDICTIONS_PATH.read_text(encoding="utf-8"))
        records = payload.get("predictions", payload)
        return {int(item["sample_id"]): int(item["predicted_prefix"]) for item in records}
    if source == "oracle":
        oracle = json.loads((ARTIFACTS_DIR / "oracle_dataset.json").read_text(encoding="utf-8"))
        return {int(sample_id): int(item["leaked_nonce_prefix"]) for sample_id, item in oracle["samples"].items()}
    raise ValueError("source must be cnn or oracle")


def build_instance(source: str, sample_count: int) -> tuple[HNPLatticeSolver, list[int], list[int], list[int]]:
    public_records = json.loads(PUBLIC_DATASET_PATH.read_text(encoding="utf-8"))
    prefixes = load_prefixes(source)
    selected = []
    for record in public_records:
        sample_id = int(record["sample_id"])
        if sample_id in prefixes:
            selected.append((record, prefixes[sample_id]))
        if len(selected) >= sample_count:
            break
    if len(selected) < sample_count:
        raise RuntimeError(f"Only {len(selected)} samples available for HNP; need {sample_count}.")

    leaked_bits = int(selected[0][0]["leaked_bits"])
    t_list = []
    u_list = []
    a_list = []
    for record, prefix in selected:
        t_i, u_i, a_i = compute_hnp_terms(record["r"], record["s"], record["z"], prefix, CURVE.n, int(record["leaked_bits"]))
        t_list.append(t_i)
        u_list.append(u_i)
        a_list.append(a_i)

    return HNPLatticeSolver(HNPConfig(leaked_bits=leaked_bits, num_samples=sample_count)), t_list, u_list, a_list


def run_hnp(source: str = "cnn", sample_count: int = 40) -> Optional[int]:
    solver, t_list, u_list, a_list = build_instance(source, sample_count)
    reduction_error = None
    if not FPYLLL_AVAILABLE:
        reduced_candidate = None
        reduction_error = (
            "fpylll is not installed; SymPy is kept as a validation-only fallback "
            "because it is unreliable for this lattice size."
        )
    else:
        try:
            reduced_candidate = solver.solve(t_list, u_list, a_list)
        except RuntimeError as exc:
            reduced_candidate = None
            reduction_error = str(exc)
    result = {
        "source": source,
        "samples": sample_count,
        "leaked_bits": solver.config.leaked_bits,
        "recovered_private_key": reduced_candidate,
        "validated": reduced_candidate is not None and solver.validate_candidate(reduced_candidate, t_list, u_list, a_list),
        "reduction_error": reduction_error,
    }
    ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)
    HNP_RESULT_PATH.write_text(json.dumps(result, indent=2), encoding="utf-8")
    if reduction_error:
        print(f"[WARN] HNP reduction was not completed: {reduction_error}")
    elif reduced_candidate is None:
        print(f"[INFO] LLL reduction completed but did not expose a validated key vector for source={source}.")
    else:
        print(f"[PASS] HNP LLL reduction returned a validated key candidate for source={source}.")
    return reduced_candidate


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", choices=("cnn", "oracle"), default="cnn")
    parser.add_argument("--samples", type=int, default=40)
    args = parser.parse_args()
    run_hnp(args.source, args.samples)


if __name__ == "__main__":
    main()
