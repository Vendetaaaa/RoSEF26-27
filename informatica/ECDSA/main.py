from __future__ import annotations

import argparse
import json
from pathlib import Path

import cnn_nonce_analysis5
import dataset_generator3
import ecdsa_leakage_model4
import diophantine_experiments7
import hnp_attack1
import theory_to_experiment_bridge8
from Lattice_key6 import FPYLLL_AVAILABLE, HNPConfig, HNPLatticeSolver

ARTIFACTS_DIR = Path(__file__).resolve().parents[1] / "artifacts"
PIPELINE_RESULT_PATH = ARTIFACTS_DIR / "pipeline_results.json"


def evaluate_hnp_status(
    recovered_key: int | None,
    relation_ok: bool,
    private_key: int,
    backend: str | None = None,
    require_fpylll: bool = False,
) -> str:
    key_ok = recovered_key is not None and recovered_key == private_key
    if key_ok and backend == "fpylll":
        return "PASS"
    if key_ok:
        return "PASS_DEVELOPMENT"
    if relation_ok:
        return "VALIDATED"
    return "FAIL"


def is_hnp_pipeline_pass(results: dict) -> bool:
    return (
        results.get("dataset") == "PASS"
        and results.get("leakage") == "PASS"
        and results.get("cnn") == "PASS"
        and results.get("hnp") == "PASS"
        and results.get("math") == "PASS"
        and results.get("fully_executed") is True
    )


def validate_shared_hnp_instance(require_fpylll: bool = False) -> dict:
    oracle = dataset_generator3.load_ground_truth()
    public_records = dataset_generator3.load_public_dataset()
    split_path = Path(__file__).resolve().parents[1] / "artifacts" / "dataset_split.json"
    if not split_path.exists():
        split_path = Path(__file__).resolve().parents[1] / "dataset" / "sample" / "dataset_split.json"
    split = json.loads(split_path.read_text(encoding="utf-8"))
    test_ids = set(int(sample_id) for sample_id in split["test"])
    prefix_by_id = {int(k): int(v["leaked_nonce_prefix"]) for k, v in oracle["samples"].items()}
    selected = [record for record in public_records if int(record["sample_id"]) in test_ids][:40]
    if len(selected) != 40:
        raise RuntimeError(f"The configured HNP attack split contains {len(selected)} samples; need exactly 40.")

    q = dataset_generator3.CURVE.n
    leaked_bits = int(selected[0]["leaked_bits"])
    hidden_bits = q.bit_length() - leaked_bits
    t_list: list[int] = []
    u_list: list[int] = []
    a_list: list[int] = []
    for record in selected:
        inverse_s = pow(int(record["s"]), -1, q)
        t_list.append(inverse_s * int(record["r"]) % q)
        u_list.append(inverse_s * int(record["z"]) % q)
        a_list.append(prefix_by_id[int(record["sample_id"])] << hidden_bits)

    private_key = int(oracle["private_key"])
    solver = HNPLatticeSolver(HNPConfig(leaked_bits=leaked_bits, num_samples=len(selected)))
    relation_ok = solver.verify_expected_vector(private_key, t_list, u_list, a_list)
    if not relation_ok:
        print("[FAIL] Shared HNP equation does not satisfy the configured lattice embedding.")
        return {"relation_ok": False, "recovered_private_key": None, "backend": None}

    recovered = None
    backend = None
    reduction_error = None
    try:
        recovered, backend = solver.solve(
            t_list, u_list, a_list, require_fpylll=require_fpylll
        )
    except RuntimeError as exc:
        reduction_error = str(exc)

    key_ok = recovered == private_key
    if key_ok:
        print(f"[PASS] Oracle HNP recovery succeeded with backend={backend}.")
    elif reduction_error:
        print(f"[FAIL] Oracle HNP reduction failed: {reduction_error}")
    else:
        print(f"[FAIL] Oracle HNP reduction completed with backend={backend}, but the key was not recovered.")
    return {
        "relation_ok": relation_ok,
        "backend": backend,
        "key_recovered": key_ok,
        "reduction_error": reduction_error,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Run the end-to-end synthetic RoSEF pipeline.")
    parser.add_argument("--require-fpylll", action="store_true", help="Require fpylll for the reference HNP execution.")
    args = parser.parse_args(argv)
    if args.require_fpylll and not FPYLLL_AVAILABLE:
        print("[FAIL] fpylll is required for reference execution but is not installed.")
        return 1

    ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)
    dataset_ok = bool(dataset_generator3.main())
    leakage_ok = bool(ecdsa_leakage_model4.main())
    cnn_ran = cnn_nonce_analysis5.run()

    private_key = int(dataset_generator3.load_ground_truth()["private_key"])
    oracle_hnp = validate_shared_hnp_instance(require_fpylll=args.require_fpylll)
    cnn_recovery = None
    hnp_result = None
    if cnn_ran and (ARTIFACTS_DIR / "cnn_predictions.json").exists():
        cnn_recovery = hnp_attack1.run_hnp("cnn", 40, require_fpylll=args.require_fpylll)
        hnp_result = json.loads((ARTIFACTS_DIR / "hnp_result.json").read_text(encoding="utf-8"))

    hnp_status = evaluate_hnp_status(
        cnn_recovery,
        bool(oracle_hnp["relation_ok"]),
        private_key,
        hnp_result.get("reduction_backend") if hnp_result else None,
        require_fpylll=args.require_fpylll,
    )

    theory_to_experiment_bridge8.main()
    diophantine_experiments7.main()
    bridge_data = json.loads(theory_to_experiment_bridge8.RESULTS_PATH.read_text(encoding="utf-8"))
    dioph_data = json.loads(diophantine_experiments7.RESULTS_PATH.read_text(encoding="utf-8"))
    math_status = "PASS" if all(value["passed"] for value in bridge_data.values()) and all(value["passed"] for value in dioph_data.values()) else "FAIL"

    results = {
        "dataset": "PASS" if dataset_ok else "FAIL",
        "leakage": "PASS" if leakage_ok else "FAIL",
        "cnn": "PASS" if cnn_ran else "SKIPPED",
        "hnp": hnp_status,
        "hnp_backend": hnp_result.get("reduction_backend") if hnp_result else None,
        "hnp_key_recovered": bool(cnn_recovery is not None),
        "oracle_hnp": oracle_hnp,
        "math": math_status,
        "fully_executed": bool(cnn_ran and hnp_status == "PASS"),
        "fpylll_available": bool(FPYLLL_AVAILABLE),
        "fpylll_required": bool(args.require_fpylll),
    }
    PIPELINE_RESULT_PATH.write_text(json.dumps(results, indent=2), encoding="utf-8")
    required_pass = is_hnp_pipeline_pass(results)
    development_pass = (
        not args.require_fpylll
        and results["dataset"] == "PASS"
        and results["leakage"] == "PASS"
        and results["cnn"] == "PASS"
        and results["math"] == "PASS"
        and results["hnp"] == "PASS_DEVELOPMENT"
        and results["hnp_key_recovered"]
    )
    print("=" * 80)
    status = "PASS" if required_pass else ("PASS_DEVELOPMENT" if development_pass else "FAIL")
    print(f"Pipeline status: {status}")
    print(f"CNN -> HNP private-key recovery: {'YES' if results['hnp_key_recovered'] else 'NO'}")
    print(f"HNP backend: {results['hnp_backend']}")
    print(f"Results written to {PIPELINE_RESULT_PATH}.")
    return 0 if (required_pass or development_pass) else 1


if __name__ == "__main__":
    raise SystemExit(main())
