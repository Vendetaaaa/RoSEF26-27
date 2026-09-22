from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ECDSA_DIR = ROOT / "informatica" / "ECDSA"

if str(ECDSA_DIR) not in sys.path:
    sys.path.insert(0, str(ECDSA_DIR))


def test_ecdsa_modules_import() -> None:
    import ecdsa_simulator2
    import ecdsa_leakage_model4
    import dataset_generator3

    assert ecdsa_simulator2 is not None
    assert ecdsa_leakage_model4 is not None
    assert dataset_generator3 is not None


def test_curve_generator_is_valid() -> None:
    from dataset_generator3 import CURVE, is_on_curve

    assert is_on_curve(CURVE, CURVE.G)


def test_group_order_identity() -> None:
    from dataset_generator3 import CURVE, INF, scalar_mult

    assert scalar_mult(CURVE, CURVE.n, CURVE.G) == INF


def test_dataset_generation_is_deterministic() -> None:
    from dataset_generator3 import CURVE, generate_dataset

    public_a, truth_a = generate_dataset(
        CURVE,
        sample_count=4,
        leaked_bits=12,
        seed=12345,
    )
    public_b, truth_b = generate_dataset(
        CURVE,
        sample_count=4,
        leaked_bits=12,
        seed=12345,
    )

    assert public_a == public_b
    assert truth_a.private_key == truth_b.private_key
