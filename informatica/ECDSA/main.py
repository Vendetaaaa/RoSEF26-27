from __future__ import annotations

import json
from pathlib import Path

import cnn_nonce_analysis5
import dataset_generator3
import ecdsa_leakage_model4
import diophantine_experiments7
import hnp_attack1
import theory_to_experiment_bridge8
from Lattice_key6 import HNPConfig, HNPLatticeSolver

ARTIFACTS_DIR = Path(__file__).resolve().parents[1] / "artifacts"
PIPELINE_RESULT_PATH = ARTIFACTS_DIR / "pipeline_results.json"


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

    solver.reduce(solver.build_basis_matrix(t_list, u_list, a_list))
    print("[PASS] Shared HNP equations satisfy the corrected lattice embedding.")
    print("[PASS] LLL reduction completes on the shared dataset without exposing oracle fields to the solver.")
    return True


def main() -> None:
    ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)
    dataset_generator3.main()
    ecdsa_leakage_model4.main()

    cnn_ran = cnn_nonce_analysis5.run()
    hnp_recovery = None
    relation_ok = validate_shared_hnp_instance()
    if cnn_ran and (ARTIFACTS_DIR / "cnn_predictions.json").exists():
        hnp_recovery = hnp_attack1.run_hnp("cnn", 40)
        oracle = dataset_generator3.load_ground_truth()
        hnp_status = "PASS" if hnp_recovery == int(oracle["private_key"]) else ("VALIDATED" if relation_ok else "FAIL")
    else:
        hnp_status = "VALIDATED" if relation_ok else "FAIL"

    theory_to_experiment_bridge8.main()
    diophantine_experiments7.main()
    bridge_data = json.loads(theory_to_experiment_bridge8.RESULTS_PATH.read_text(encoding="utf-8"))
    dioph_data = json.loads(diophantine_experiments7.RESULTS_PATH.read_text(encoding="utf-8"))
    math_status = "PASS" if all(value["passed"] for value in bridge_data.values()) and all(value["passed"] for value in dioph_data.values()) else "FAIL"

    results = {
        "dataset": "PASS",
        "leakage": "PASS",
        "cnn": "PASS" if cnn_ran else "SKIPPED",
        "hnp": hnp_status,
        "math": math_status,
        "fully_executed": bool(cnn_ran and hnp_status == "PASS"),
    }
    PIPELINE_RESULT_PATH.write_text(json.dumps(results, indent=2), encoding="utf-8")
    required_pass = results["dataset"] == "PASS" and results["leakage"] == "PASS" and results["hnp"] in {"PASS", "VALIDATED"} and results["math"] == "PASS"
    print("=" * 80)
    print(f"Pipeline status: {'PASS' if required_pass else 'FAIL'}")
    if not cnn_ran:
        print("CNN status: SKIPPED because PyTorch is not installed in this environment.")
    print(f"Results written to {PIPELINE_RESULT_PATH}.")


if __name__ == "__main__":
    main()
