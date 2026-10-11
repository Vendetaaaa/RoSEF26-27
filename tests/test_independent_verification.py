from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ECDSA_DIR = ROOT / "informatica" / "ECDSA"

if str(ECDSA_DIR) not in sys.path:
    sys.path.insert(0, str(ECDSA_DIR))


def test_public_key_verifies_correct_candidate() -> None:
    from dataset_generator3 import CURVE, generate_dataset, public_dataset_as_dicts, scalar_mult
    from independent_verification import verify_candidate

    samples, truth = generate_dataset(CURVE, sample_count=3, leaked_bits=12, seed=12345)
    public_key = scalar_mult(CURVE, truth.private_key, CURVE.G)
    records = public_dataset_as_dicts(samples, public_key)

    assert verify_candidate(truth.private_key, records)


def test_public_key_rejects_wrong_candidate() -> None:
    from dataset_generator3 import CURVE, generate_dataset, public_dataset_as_dicts, scalar_mult
    from independent_verification import verify_candidate

    samples, truth = generate_dataset(CURVE, sample_count=3, leaked_bits=12, seed=12345)
    public_key = scalar_mult(CURVE, truth.private_key, CURVE.G)
    records = public_dataset_as_dicts(samples, public_key)
    wrong_key = (truth.private_key % (CURVE.n - 1)) + 1

    assert wrong_key != truth.private_key
    assert not verify_candidate(wrong_key, records)


def test_public_key_verification_rejects_inconsistent_records() -> None:
    from dataset_generator3 import CURVE, generate_dataset, public_dataset_as_dicts, scalar_mult
    from independent_verification import verify_candidate

    samples, truth = generate_dataset(CURVE, sample_count=3, leaked_bits=12, seed=12345)
    public_key = scalar_mult(CURVE, truth.private_key, CURVE.G)
    records = public_dataset_as_dicts(samples, public_key)
    records[1]["public_key"]["x"] = "00"

    assert not verify_candidate(truth.private_key, records)
def test_hnp_status_requires_public_key_verification_for_pass():
    from main import evaluate_hnp_status

    assert evaluate_hnp_status(
        recovered_key=123,
        relation_ok=True,
        private_key=123,
        backend="fpylll",
        public_key_ok=False,
    ) == "VALIDATED"


def test_hnp_status_passes_with_public_key_verification():
    from main import evaluate_hnp_status

    assert evaluate_hnp_status(
        recovered_key=123,
        relation_ok=True,
        private_key=123,
        backend="fpylll",
        public_key_ok=True,
    ) == "PASS"

def test_hnp_status_does_not_pass_without_recovered_key():
    from main import evaluate_hnp_status

    assert evaluate_hnp_status(
        recovered_key=None,
        relation_ok=True,
        private_key=123,
        backend="fpylll",
        public_key_ok=True,
    ) == "VALIDATED"


def test_hnp_status_fails_if_fpylll_is_required_but_not_used():
    from main import evaluate_hnp_status

    assert evaluate_hnp_status(
        recovered_key=123,
        relation_ok=True,
        private_key=123,
        backend="sympy",
        require_fpylll=True,
        public_key_ok=True,
    ) == "FAIL"


def test_public_key_verification_rejects_boolean_candidate() -> None:
    from independent_verification import verify_candidate

    assert not verify_candidate(True, [{"public_key": {"curve": "secp256k1", "x": "1", "y": "1"}}])
    assert not verify_candidate(False, [{"public_key": {"curve": "secp256k1", "x": "1", "y": "1"}}])


def test_public_key_verification_rejects_malformed_public_key_record() -> None:
    from independent_verification import verify_candidate

    assert not verify_candidate(1, [{"public_key": None}])
