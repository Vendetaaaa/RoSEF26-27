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
    if any(int(record["leaked_bits"]) != leaked_bits for record, _ in selected):
        raise RuntimeError("HNP attack samples must use the same leaked-bit width.")

    t_list: list[int] = []
    u_list: list[int] = []
    a_list: list[int] = []
    for record, prefix in selected:
        t_i, u_i, a_i = compute_hnp_terms(
            record["r"], record["s"], record["z"], prefix, CURVE.n, leaked_bits
        )
        t_list.append(t_i)
        u_list.append(u_i)
        a_list.append(a_i)

    solver = HNPLatticeSolver(HNPConfig(leaked_bits=leaked_bits, num_samples=sample_count))
    return solver, t_list, u_list, a_list


def run_hnp(
    source: str = "cnn",
    sample_count: int = 40,
    *,
    require_fpylll: bool = False,
) -> Optional[int]:
    solver, t_list, u_list, a_list = build_instance(source, sample_count)
    reduction_error = None
    recovered = None
    reduction_backend = "unavailable"
    try:
        recovered, reduction_backend = solver.solve(
            t_list, u_list, a_list, require_fpylll=require_fpylll
        )
    except RuntimeError as exc:
        reduction_error = str(exc)

    validated = recovered is not None and solver.validate_candidate(
        recovered, t_list, u_list, a_list
    )
    result = {
        "source": source,
        "samples": sample_count,
        "leaked_bits": solver.config.leaked_bits,
        "reduction_backend": reduction_backend,
        "backend_policy": "require_fpylll" if require_fpylll else "auto",
        "fpylll_available": bool(FPYLLL_AVAILABLE),
        "reduction_executed": reduction_error is None and reduction_backend in {"fpylll", "sympy"},
        "key_recovered": bool(recovered is not None),
        "recovered_private_key": recovered,
        "validated": validated,
        "reduction_error": reduction_error,
    }
    ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)
    HNP_RESULT_PATH.write_text(json.dumps(result, indent=2), encoding="utf-8")

    if reduction_error:
        print(f"[FAIL] HNP reduction was not completed: {reduction_error}")
    elif recovered is None:
        print(
            f"[FAIL] LLL reduction completed with backend={reduction_backend}, "
            f"but did not expose a validated key candidate for source={source}."
        )
    else:
        policy = "fpylll" if require_fpylll else reduction_backend
        print(
            f"[PASS] HNP LLL reduction returned a validated key candidate for source={source} "
            f"using {reduction_backend} (policy={policy})."
        )
    return recovered


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", choices=("cnn", "oracle"), default="cnn")
    parser.add_argument("--samples", type=int, default=40)
    parser.add_argument(
        "--require-fpylll", action="store_true",
        help="Refuse the SymPy fallback; use this mode for the reference CI run.",
    )
    args = parser.parse_args()
    run_hnp(args.source, args.samples, require_fpylll=args.require_fpylll)


if __name__ == "__main__":
    main()
