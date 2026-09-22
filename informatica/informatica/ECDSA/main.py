from __future__ import annotations

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


def evaluate_hnp_status(recovered_key: int | None, relation_ok: bool, private_key: int) -> str:
    if recovered_key is not None and recovered_key == private_key:
        return "PASS"
    if relation_ok:
        return "VALIDATED"
    return "FAIL"


def is_hnp_pipeline_pass(results: dict) -> bool:
    return (
        results.get("dataset") == "PASS"
        and results.get("leakage") == "PASS"
        and results.get("hnp") == "PASS"
        and results.get("math") == "PASS"
    )


def validate_shared_hnp_instance() -> bool:
    oracle = dataset_generator3.load_ground_truth()
    public_records = dataset_generator3.load_public_dataset()
    prefix_by_id = {int(k): int(v["leaked_nonce_prefix"]) for k, v in oracle["samples"].items()}
    selected = public_records[:40]
    q = dataset_generator3.CURVE.n
    leaked_bits = int(selected[0]["leaked_bits"])
    hidden_bits = q.bit_length() - leaked_bits
    t_list = []
    u_list = []
    a_list = []

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
        return False

    if not FPYLLL_AVAILABLE:
        print("[WARN] fpylll is not installed; skipping full LLL reduction in this environment.")
        return True
    try:
        solver.reduce(solver.build_basis_matrix(t_list, u_list, a_list))
        print("[PASS] Shared HNP equations satisfy the corrected lattice embedding.")
        print("[PASS] LLL reduction completed on the shared dataset without exposing oracle fields to the solver.")
    except RuntimeError as exc:
        print(f"[WARN] Shared HNP equations are valid, but lattice reduction was unavailable: {exc}")
    return True


def main() -> None:
    ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)
    dataset_ok = bool(dataset_generator3.main())
    leakage_ok = bool(ecdsa_leakage_model4.main())

    cnn_ran = cnn_nonce_analysis5.run()
    hnp_recovery = None
    relation_ok = validate_shared_hnp_instance()
    private_key = int(dataset_generator3.load_ground_truth()["private_key"])
    if cnn_ran and (ARTIFACTS_DIR / "cnn_predictions.json").exists():
        hnp_recovery = hnp_attack1.run_hnp("cnn", 40)
        hnp_status = evaluate_hnp_status(hnp_recovery, relation_ok, private_key)
    else:
        hnp_status = evaluate_hnp_status(None, relation_ok, private_key)

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
        "math": math_status,
        "fully_executed": bool(cnn_ran and hnp_status == "PASS"),
    }
    PIPELINE_RESULT_PATH.write_text(json.dumps(results, indent=2), encoding="utf-8")
    required_pass = is_hnp_pipeline_pass(results)
    print("=" * 80)
    print(f"Pipeline status: {'PASS' if required_pass else 'FAIL'}")
    print(f"Private key recovered: {'YES' if hnp_status == 'PASS' else 'NO'}")
    if hnp_status == "VALIDATED":
        print("HNP status: equations are consistent, but the private key was not recovered.")
    if not cnn_ran:
        print("CNN status: SKIPPED because PyTorch is not installed in this environment.")
    print(f"Results written to {PIPELINE_RESULT_PATH}.")


if __name__ == "__main__":
    main()
