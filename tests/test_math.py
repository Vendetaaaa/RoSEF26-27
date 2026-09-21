from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ECDSA_DIR = ROOT / "informatica" / "ECDSA"
if str(ECDSA_DIR) not in sys.path:
    sys.path.insert(0, str(ECDSA_DIR))

from Lattice_key6 import HNPConfig, HNPLatticeSolver
from diophantine_experiments7 import count_k3_bruteforce, count_k3_formula
from dataset_generator3 import CURVE, generate_dataset


def test_k3_formula_matches_bruteforce() -> None:
    box = range(-6, 7)
    assert count_k3_bruteforce(box) == count_k3_formula(box)


def test_curve_group_order() -> None:
    from dataset_generator3 import INF, is_on_curve, scalar_mult
    assert is_on_curve(CURVE, CURVE.G)
    assert scalar_mult(CURVE, CURVE.n, CURVE.G) == INF


def test_hnp_embedding_matches_ground_truth() -> None:
    public, truth = generate_dataset(CURVE, sample_count=8, leaked_bits=12, seed=1234)
    q = CURVE.n
    hidden = q.bit_length() - 12
    truth_by_id = {sample.sample_id: sample for sample in truth.samples}
    t, u, a = [], [], []
    for record in public:
        inv_s = pow(record.s, -1, q)
        t.append(inv_s * record.r % q)
        u.append(inv_s * record.z % q)
        a.append(truth_by_id[record.sample_id].leaked_nonce_prefix << hidden)
    solver = HNPLatticeSolver(HNPConfig(leaked_bits=12, num_samples=8))
    assert solver.verify_expected_vector(truth.private_key, t, u, a)
