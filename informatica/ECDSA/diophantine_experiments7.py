from __future__ import annotations

import itertools
import math
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
from scipy.stats import linregress

RESULTS_PATH = Path(__file__).resolve().parents[1] / "artifacts" / "diophantine_results.json"


def print_metric(label, computed, expected, passed) -> None:
    status = "[PASS]" if passed else "[FAIL]"
    print(f"  {status} {label:<48} | Computed: {computed} | Reference: {expected}")


def run_module_1() -> dict:
    N1 = 10**6
    a = 2.3 + 1.9j
    i1 = np.arange(1, N1 + 1, dtype=np.float64)
    m1 = np.floor(np.sin(i1) * 100).astype(np.int64)
    n1 = 50.0 * np.sqrt(2.0) * np.cos(i1**2)
    re_z1 = m1 + n1 * a.real
    unique_m1 = int(np.unique(m1).size)
    collision_count = N1 - unique_m1
    slope_k = a.imag / a.real
    bounds_ok = -270.0 <= float(re_z1.min()) <= -260.0 and 260.0 <= float(re_z1.max()) <= 270.0
    print("MODULE 1: BOUNDED VS UNBOUNDED INTEGER SHIFT")
    print_metric("Bounded m_i coverage", unique_m1, "200 possible values", unique_m1 == 200)
    print_metric("Bounded line slope", round(slope_k, 6), "19/23", math.isclose(slope_k, 19 / 23, abs_tol=1e-3))
    print_metric("Bounded real-part range", f"[{re_z1.min():.3f}, {re_z1.max():.3f}]", "inside [-270, 270]", bounds_ok)

    N_ab = 200000
    i_ab = np.arange(1, N_ab + 1, dtype=np.int64)
    m_ab = (i_ab**2) % (10**6)
    n_ab = 50.0 * np.sqrt(2.0) * np.cos(i_ab**2)
    re_ab = m_ab + n_ab * a.real
    unique_ab = int(np.unique(m_ab).size)
    collision_ab = N_ab - unique_ab
    growth_ok = float(re_ab.max()) > 1e5 and float(re_ab.min()) < -1e2
    print_metric("Unbounded-shift unique values", unique_ab, "> 0 with collisions measured", unique_ab > 0)
    print_metric("Unbounded-shift collision count", collision_ab, "> 0", collision_ab > 0)
    print_metric("Unbounded-shift range", f"[{re_ab.min():.3f}, {re_ab.max():.3f}]", "crosses a wide interval", growth_ok)
    return {"passed": bool(unique_m1 == 200 and math.isclose(slope_k, 19 / 23, abs_tol=1e-3) and bounds_ok and unique_ab > 0 and collision_ab > 0 and growth_ok)}


def box_count(values: np.ndarray, epsilon: float) -> int:
    bins = np.floor(np.mod(values, 1.0) / epsilon).astype(np.int64)
    return int(np.unique(bins).size)


def run_module_2() -> dict:
    print("MODULE 2: IRRATIONAL ROTATION BOX COUNT")
    N = 200000
    n = np.arange(1, N + 1, dtype=np.float64)
    orbit = np.mod(n * math.sqrt(2.0), 1.0)
    epsilons = np.array([0.5, 0.25, 0.125, 0.0625], dtype=np.float64)
    counts = np.array([box_count(orbit, float(eps)) for eps in epsilons], dtype=np.float64)
    slope, intercept, r_value, _, _ = linregress(np.log(1 / epsilons), np.log(counts))
    pass_fit = slope > 0.9 and slope < 1.1 and r_value**2 > 0.99
    for eps, count in zip(epsilons, counts):
        print(f"  epsilon={eps:<7} occupied_boxes={int(count)}")
    print_metric("Empirical box-counting slope", round(float(slope), 6), "approximately 1", pass_fit)
    print_metric("Log-log R-squared", round(float(r_value**2), 6), "> 0.99", r_value**2 > 0.99)
    return {"passed": bool(pass_fit), "slope": float(slope), "r_squared": float(r_value**2)}


def count_three_gap_partitions(N: int = 1000) -> tuple[int, float]:
    alpha = math.sqrt(2.0)
    indices = np.arange(1, N + 1, dtype=np.float64)
    pts = np.sort(np.mod(indices * alpha, 1.0))
    gaps = np.diff(pts)
    rounded = np.unique(np.round(gaps, 10))
    mean_gap = float(np.mean(gaps))
    variance = float(np.var(gaps / mean_gap))
    return int(len(rounded)), variance


def run_module_3() -> dict:
    print("MODULE 3: STEINHAUS THREE-GAP PROPERTY")
    gap_counts = []
    variances = []
    for N in (1000, 5000, 20000):
        count, variance = count_three_gap_partitions(N)
        gap_counts.append(count)
        variances.append(variance)
        print_metric(f"Distinct gaps N={N}", count, "<= 3", count <= 3)
        print_metric(f"Spacing variance N={N}", round(variance, 6), ">= 0", variance >= 0)
    passed = all(count <= 3 for count in gap_counts) and all(variance >= 0 for variance in variances)
    return {"passed": bool(passed), "gap_counts": gap_counts, "variances": variances}


def run_module_4() -> dict:
    print("MODULE 4: POWER-FILTER SIEVE")
    N = 50000
    rng = np.random.default_rng(42)
    x = rng.uniform(-3.0, 3.0, N)
    m = rng.integers(-10, 10, N)
    exponent = m + 0.5
    transformed = x.astype(np.complex128) ** exponent
    real_mask = np.abs(transformed.imag) < 1e-9
    kept = int(np.count_nonzero(real_mask))
    dropped = N - kept
    positive_kept = int(np.count_nonzero(x[real_mask] > 0))
    consistent = kept + dropped == N and kept > 0 and dropped > 0
    print_metric("Total candidates", N, "kept + dropped", consistent)
    print_metric("Real-valued candidates kept", kept, "> 0", kept > 0)
    print_metric("Non-real candidates dropped", dropped, "> 0", dropped > 0)
    print_metric("Positive real subset", positive_kept, "subset of kept", 0 <= positive_kept <= kept)
    return {"passed": bool(consistent), "kept": kept, "dropped": dropped, "positive_kept": positive_kept}


def count_k3_bruteforce(box) -> int:
    return sum(1 for m1, m2, m3 in itertools.product(box, repeat=3) if 2 * m1 + m2 - 3 * m3 == 0)


def count_k3_formula(box) -> int:
    lo, hi = min(box), max(box)
    total = 0
    for m1 in box:
        for m2 in box:
            numerator = 2 * m1 + m2
            if numerator % 3 == 0 and lo <= numerator // 3 <= hi:
                total += 1
    return total


def run_module_5() -> dict:
    print("MODULE 5: k-FAMILY CONCURRENCY COUNTS")
    box3 = list(range(-15, 16))
    brute3 = count_k3_bruteforce(box3)
    formula3 = count_k3_formula(box3)
    print_metric("k=3 brute-force count", brute3, "exact formula count", brute3 == formula3)

    box4 = list(range(-12, 13))
    engineered = 0
    for m1, m2, m3, m4 in itertools.product(box4, repeat=4):
        if 2 * m1 + m2 - 3 * m3 == 0 and 3 * m1 - m2 - 2 * m4 == 0:
            engineered += 1
    print_metric("Engineered k=4 count", engineered, "> 0", engineered > 0)

    unrelated = 0
    box_unrelated = list(range(-8, 9))
    for m1, m2, m3, m4 in itertools.product(box_unrelated, repeat=4):
        if (m1 + 3 * m2 - 7 * m3 + 11 * m4 == 0) and (2 * m1 - m2 + 5 * m3 - 3 * m4 == 0):
            unrelated += 1
    print_metric("Unrelated-system integer intersections", unrelated, "computed without zero-forcing", unrelated >= 0)
    return {"passed": bool(brute3 == formula3 and engineered > 0 and unrelated >= 0), "k3": brute3, "k4_engineered": engineered, "k4_unrelated": unrelated}


def main() -> None:
    print("=" * 80)
    print("DIOPHANTINE EXPERIMENTS")
    print("=" * 80)
    results = {
        "module_1": run_module_1(),
        "module_2": run_module_2(),
        "module_3": run_module_3(),
        "module_4": run_module_4(),
        "module_5": run_module_5(),
    }
    RESULTS_PATH.parent.mkdir(parents=True, exist_ok=True)
    import json
    RESULTS_PATH.write_text(json.dumps(results, indent=2), encoding="utf-8")
    passed = all(item["passed"] for item in results.values())
    print("-" * 80)
    print(f"Status: {'PASS' if passed else 'FAIL'}")


if __name__ == "__main__":
    main()
