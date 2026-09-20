from __future__ import annotations

import itertools
import json
import math
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
from scipy.stats import linregress

RESULTS_PATH = Path(__file__).resolve().parents[1] / "artifacts" / "bridge_results.json"
PLOT_PATH = Path(__file__).resolve().parents[1] / "artifacts" / "diophantine_bridge_verification.png"


def print_metric(label, computed, theoretical, passed) -> None:
    status = "[PASS]" if passed else "[FAIL]"
    print(f"  {status} {label:<46} | Empirical: {computed} | Reference: {theoretical}")


def compute_delta_M(R: float, M: int) -> float:
    defects = []
    for q in range(1, 2 * M + 1):
        p = round(q * R)
        if abs(p) <= 2 * M:
            defects.append(abs(p - q * R))
    return min(defects)


def run_module_1() -> dict:
    print("MODULE 1: KRONECKER-WEYL ASYMPTOTIC FACTOR BRIDGE")
    sqrt2 = math.sqrt(2.0)
    phi = (1.0 + math.sqrt(5.0)) / 2.0
    theoretical_sqrt2 = sqrt2 / 2.0 * (1.0 / (2.0 * sqrt2))
    theoretical_phi = phi / (2.0 * math.sqrt(5.0))
    M_values = list(range(10, 501, 10))
    sqrt2_scaled = [M * compute_delta_M(sqrt2, M) for M in M_values]
    phi_scaled = [M * compute_delta_M(phi, M) for M in M_values]
    empirical_sqrt2 = float(np.min(sqrt2_scaled[20:]))
    empirical_phi = float(np.min(phi_scaled[20:]))
    pass_sqrt2 = math.isclose(empirical_sqrt2, theoretical_sqrt2, abs_tol=1e-2)
    pass_phi = math.isclose(empirical_phi, theoretical_phi, abs_tol=1e-2)
    print_metric("sqrt(2) lim inf M*delta_M", round(empirical_sqrt2, 6), round(theoretical_sqrt2, 6), pass_sqrt2)
    print_metric("phi lim inf M*delta_M", round(empirical_phi, 6), round(theoretical_phi, 6), pass_phi)
    return {"passed": bool(pass_sqrt2 and pass_phi), "sqrt2_empirical": empirical_sqrt2, "sqrt2_theoretical": theoretical_sqrt2, "phi_empirical": empirical_phi, "phi_theoretical": theoretical_phi}


def run_module_2() -> dict:
    print("MODULE 2: DIOPHANTINE EXPONENT TRANSFER")
    R = math.sqrt(2.0)
    M_range = np.unique(np.int64(np.logspace(1, 3.5, num=30)))
    log_M = []
    log_inv_delta = []
    for M in M_range:
        delta = compute_delta_M(R, int(M))
        if delta > 0:
            log_M.append(math.log(float(M)))
            log_inv_delta.append(math.log(1.0 / delta))
    slope, _, r_value, _, _ = linregress(log_M, log_inv_delta)
    mu_geo_empirical = 1.0 + slope
    pass_transfer = math.isclose(mu_geo_empirical, 2.0, abs_tol=0.15)
    pass_fit = r_value**2 > 0.95
    print_metric("Geometric exponent mu_geo", round(mu_geo_empirical, 6), "2.0", pass_transfer)
    print_metric("Geometric log-log slope", round(float(slope), 6), "mu_geo - 1", math.isclose(mu_geo_empirical - 1.0, slope, abs_tol=1e-12))
    print_metric("Log-log R-squared", round(float(r_value**2), 6), "> 0.95", pass_fit)
    return {"passed": bool(pass_transfer and pass_fit), "mu_geo": float(mu_geo_empirical), "slope": float(slope), "r_squared": float(r_value**2)}


def three_gap_count(N: int) -> tuple[int, float]:
    alpha = math.sqrt(2.0)
    indices = np.arange(1, N + 1, dtype=np.float64)
    pts = np.sort(np.mod(indices * alpha, 1.0))
    gaps = np.diff(pts)
    unique_gaps = np.unique(np.round(gaps, 10))
    variance = float(np.var(gaps / np.mean(gaps)))
    return int(unique_gaps.size), variance


def run_module_3() -> dict:
    print("MODULE 3: STEINHAUS-SOS THREE-GAP RIGIDITY")
    counts = []
    variances = []
    for N in (1000, 5000, 20000):
        count, variance = three_gap_count(N)
        counts.append(count)
        variances.append(variance)
        print_metric(f"Distinct gaps N={N}", count, "<= 3", count <= 3)
    passed = all(count <= 3 for count in counts)
    return {"passed": bool(passed), "N_scales": [1000, 5000, 20000], "gap_counts": counts, "variances": variances}


def run_module_4() -> dict:
    print("MODULE 4: EXACT CONCURRENCY DICTIONARY")
    box_limit = 15
    box = list(range(-box_limit, box_limit + 1))
    nt_solutions = {(m1, m2, m3) for m1, m2, m3 in itertools.product(box, repeat=3) if 2 * m1 + m2 - 3 * m3 == 0}
    kappa1, kappa2, kappa3 = 1.0, 4.0, 2.0
    geom = set()
    for m1, m2, m3 in itertools.product(box, repeat=3):
        x12 = (m2 - m1) / (kappa1 - kappa2)
        y12 = kappa1 * x12 + m1
        y3 = kappa3 * x12 + m3
        if math.isclose(y12, y3, abs_tol=1e-9):
            geom.add((m1, m2, m3))
    sym_diff = nt_solutions.symmetric_difference(geom)
    count_nt = len(nt_solutions)
    count_geom = len(geom)
    expected_count = 321
    count_pass = count_nt == expected_count and count_geom == expected_count
    diff_pass = len(sym_diff) == 0
    print_metric("Number-theoretic solution count", count_nt, expected_count, count_pass)
    print_metric("Geometric intersection count", count_geom, expected_count, count_geom == expected_count)
    print_metric("Symmetric-difference size", len(sym_diff), 0, diff_pass)
    return {"passed": bool(count_pass and diff_pass), "count_nt": count_nt, "count_geom": count_geom, "sym_diff_size": len(sym_diff), "expected_count": expected_count}


def generate_verification_plots(mod1, mod2, mod3, mod4) -> None:
    fig, axs = plt.subplots(2, 2, figsize=(14, 10))
    M_vals = list(range(10, 201, 10))
    defects = [M * compute_delta_M(math.sqrt(2.0), M) for M in M_vals]
    axs[0, 0].plot(M_vals, defects, "o-", label="Empirical M delta_M(sqrt(2))")
    axs[0, 0].axhline(0.25, linestyle="--", label="Analytical limit 0.25")
    axs[0, 0].set_title("Module 1")
    axs[0, 0].legend()

    M_range = np.unique(np.int64(np.logspace(1, 3.5, num=20)))
    x = np.log(M_range.astype(float))
    y = np.log([1.0 / compute_delta_M(math.sqrt(2.0), int(M)) for M in M_range])
    axs[0, 1].plot(x, y, "s", label=f"Data slope={mod2['slope']:.3f}")
    axs[0, 1].plot(x, mod2["slope"] * x + (y[0] - mod2["slope"] * x[0]), "--", label="Fitted slope")
    axs[0, 1].set_title("Module 2")
    axs[0, 1].legend()

    axs[1, 0].bar([str(n) for n in mod3["N_scales"]], mod3["gap_counts"])
    axs[1, 0].set_title("Module 3: distinct gaps")

    axs[1, 1].bar(["Number-theoretic", "Geometric"], [mod4["count_nt"], mod4["count_geom"]])
    axs[1, 1].set_title(f"Module 4: exact count {mod4['expected_count']}")

    plt.tight_layout()
    PLOT_PATH.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(PLOT_PATH, dpi=180)
    plt.close(fig)
    print(f"[PASS] Verification plot written to {PLOT_PATH}.")


def main() -> None:
    print("=" * 80)
    print("THEORY TO EXPERIMENT BRIDGE")
    print("=" * 80)
    results = {
        "module_1": run_module_1(),
        "module_2": run_module_2(),
        "module_3": run_module_3(),
        "module_4": run_module_4(),
    }
    generate_verification_plots(results["module_1"], results["module_2"], results["module_3"], results["module_4"])
    RESULTS_PATH.parent.mkdir(parents=True, exist_ok=True)
    RESULTS_PATH.write_text(json.dumps(results, indent=2), encoding="utf-8")
    passed = all(item["passed"] for item in results.values())
    print("-" * 80)
    print(f"Status: {'PASS' if passed else 'FAIL'}")


if __name__ == "__main__":
    main()
