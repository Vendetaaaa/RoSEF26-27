"""Independent secp256k1 public-key and ECDSA signature verification.

Uses OpenSSL through the ``cryptography`` package rather than the project's
own elliptic-curve arithmetic. All records are synthetic research fixtures.
"""
from __future__ import annotations

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec, utils

from dataset_generator3 import CURVE


def verify_candidate(candidate_private_key: int, public_records: list[dict]) -> bool:
    """Verify candidate public key and every provided ECDSA signature independently.

    Returns False for malformed/inconsistent records, invalid candidates, a public-key
    mismatch, or any signature that fails verification.
    """
    if isinstance(candidate_private_key, bool) or not isinstance(candidate_private_key, int):
        return False
    if not 1 <= candidate_private_key < CURVE.n or not public_records:
        return False

    try:
        candidate = ec.derive_private_key(candidate_private_key, ec.SECP256K1())
        candidate_numbers = candidate.public_key().public_numbers()
        first_key = public_records[0]["public_key"]
        if not isinstance(first_key, dict) or first_key.get("curve") != "secp256k1":
            return False
        expected_x, expected_y = int(first_key["x"], 16), int(first_key["y"], 16)
        if (candidate_numbers.x, candidate_numbers.y) != (expected_x, expected_y):
            return False
        public_key = ec.EllipticCurvePublicNumbers(
            expected_x, expected_y, ec.SECP256K1()
        ).public_key()

        for record in public_records:
            key = record.get("public_key")
            if not isinstance(key, dict) or key.get("curve") != "secp256k1":
                return False
            if (int(key["x"], 16), int(key["y"], 16)) != (expected_x, expected_y):
                return False
            r, s = int(record["r"]), int(record["s"])
            if not (1 <= r < CURVE.n and 1 <= s < CURVE.n):
                return False
            message = record["message"]
            if isinstance(message, str):
                message = message.encode("utf-8")
            if not isinstance(message, bytes):
                return False
            der_signature = utils.encode_dss_signature(r, s)
            public_key.verify(der_signature, message, ec.ECDSA(hashes.SHA256()))
        return True
    except (InvalidSignature, ValueError, TypeError, KeyError, AttributeError, OverflowError):
        return False
