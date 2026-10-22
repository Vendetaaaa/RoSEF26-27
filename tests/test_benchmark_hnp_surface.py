from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXPERIMENTS = ROOT / "informatica" / "experiments"
if str(EXPERIMENTS) not in sys.path:
    sys.path.insert(0, str(EXPERIMENTS))

import benchmark_cnn_hnp_surface as benchmark


def test_hnp_solver_runs_before_ground_truth_diagnostic(monkeypatch) -> None:
    events: list[str] = []

    class FakeSolver:
        def __init__(self, config) -> None:
            self.config = config

        def solve(self, t_list, u_list, a_list, *, require_fpylll=False):
            events.append("solve")
            return None, "fpylll"

        def verify_expected_vector(self, private_key, t_list, u_list, a_list):
            events.append("ground_truth_check")
            return False

    monkeypatch.setattr(benchmark, "HNPLatticeSolver", FakeSolver)
    result = benchmark.solve_hnp(
        public_by_id={0: {"r": 1, "s": 1, "z": 1}},
        prefixes={0: 0},
        sample_ids=[0],
        leaked_bits=12,
        private_key=123,
        require_fpylll=True,
    )

    assert events == ["solve", "ground_truth_check"]
    assert result["relation_ok"] is False
    assert result["key_recovered"] is False
    assert result["backend"] == "fpylll"
    assert result["runtime_seconds"] is not None


def test_model_mode_routes_results_to_its_own_directory(monkeypatch) -> None:
    captured = {}

    def fake_run_multi_seed_surface(*args, **kwargs):
        captured.update(kwargs)
        return [], []

    monkeypatch.setattr(benchmark, "run_multi_seed_surface", fake_run_multi_seed_surface)
    monkeypatch.setattr(sys, "argv", ["benchmark", "--leakage-mode", "HD"])

    assert benchmark.main() == 0
    assert captured["leakage_mode"] == "HD"
    assert captured["output_path"].as_posix().endswith(
        "results/synthetic/HD/tables/cnn-hnp-parameter-sweep.csv"
    )


def test_profile_is_tagged_with_the_selected_leakage_model() -> None:
    from dataset_generator3 import CURVE, generate_dataset

    public, truth = generate_dataset(CURVE, sample_count=2, leaked_bits=8, seed=1234)
    hw = benchmark.make_profile(public, truth, sigma=0.0, seed=7, leaked_bits=8, leakage_mode="HW")
    hd = benchmark.make_profile(public, truth, sigma=0.0, seed=7, leaked_bits=8, leakage_mode="HD")
    temporal = benchmark.make_profile(
        public, truth, sigma=0.0, seed=7, leaked_bits=8, leakage_mode="TEMPORAL"
    )

    assert {row["leakage_model"] for row in hw} == {"HW"}
    assert {row["leakage_model"] for row in hd} == {"HD"}
    assert {row["leakage_model"] for row in temporal} == {"TEMPORAL"}
    assert all(row["trace_source"] == "synthetic" for row in hw + hd + temporal)
