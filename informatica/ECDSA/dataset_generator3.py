from __future__ import annotations

import hashlib
import json
import random
from dataclasses import dataclass
from pathlib import Path
from typing import Optional


@dataclass(frozen=True)
class Point:
    x: Optional[int]
    y: Optional[int]

    @property
    def is_infinity(self) -> bool:
        return self.x is None and self.y is None


INF = Point(None, None)


@dataclass(frozen=True)
class Curve:
    p: int
    a: int
    b: int
    G: Point
    n: int


CURVE = Curve(
    p=0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F,
    a=0,
    b=7,
    G=Point(
        0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798,
        0x483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8,
    ),
    n=0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141,
)


ARTIFACTS_DIR = Path(__file__).resolve().parents[1] / "artifacts"
PUBLIC_DATASET_PATH = ARTIFACTS_DIR / "public_dataset.json"
ORACLE_DATASET_PATH = ARTIFACTS_DIR / "oracle_dataset.json"


def point_add(curve: Curve, P: Point, Q: Point) -> Point:
    if P.is_infinity:
        return Q
    if Q.is_infinity:
        return P

    p = curve.p
    if P.x == Q.x and (P.y + Q.y) % p == 0:
        return INF

    if P == Q:
        if P.y % p == 0:
            return INF
        numerator = (3 * P.x * P.x + curve.a) % p
        denominator = pow(2 * P.y, -1, p)
    else:
        numerator = (Q.y - P.y) % p
        denominator = pow(Q.x - P.x, -1, p)

    lam = (numerator * denominator) % p
    x3 = (lam * lam - P.x - Q.x) % p
    y3 = (lam * (P.x - x3) - P.y) % p
    return Point(x3, y3)


def scalar_mult(curve: Curve, k: int, P: Point) -> Point:
    if k < 0:
        raise ValueError("Scalar must be non-negative.")

    result = INF
    addend = P
    scalar = k
    while scalar:
        if scalar & 1:
            result = point_add(curve, result, addend)
        addend = point_add(curve, addend, addend)
        scalar >>= 1
    return result


def is_on_curve(curve: Curve, P: Point) -> bool:
    if P.is_infinity:
        return True
    return (P.y * P.y - (P.x * P.x * P.x + curve.a * P.x + curve.b)) % curve.p == 0


def hash_message(message: bytes, n: int) -> int:
    return int.from_bytes(hashlib.sha256(message).digest(), "big") % n


def ecdsa_sign(curve: Curve, private_key: int, z: int, k: int) -> tuple[int, int]:
    n = curve.n
    if not 1 <= private_key < n:
        raise ValueError("Invalid private key.")
    if not 1 <= k < n:
        raise ValueError("Invalid nonce.")

    R = scalar_mult(curve, k, curve.G)
    if R.is_infinity:
        raise ValueError("Invalid nonce produced infinity.")

    r = R.x % n
    if r == 0:
        raise ValueError("Invalid nonce produced r = 0.")

    s = (pow(k, -1, n) * (z + r * private_key)) % n
    if s == 0:
        raise ValueError("Invalid nonce produced s = 0.")
    return r, s


def nonce_with_known_msb(n: int, leaked_bits: int, rng: random.Random) -> tuple[int, int]:
    if not 0 <= leaked_bits < n.bit_length():
        raise ValueError("leaked_bits must be in [0, bit_length(n)-1].")

    hidden_bits = n.bit_length() - leaked_bits
    bound = 1 << hidden_bits
    prefix_limit = 1 << leaked_bits
    prefix = rng.randrange(prefix_limit) if leaked_bits else 0
    low = rng.randrange(bound)
    k = (prefix << hidden_bits) | low
    if not 1 <= k < n:
        return nonce_with_known_msb(n, leaked_bits, rng)
    return k, prefix


@dataclass(frozen=True)
class PublicSample:
    sample_id: int
    message: bytes
    z: int
    r: int
    s: int
    leaked_bits: int


@dataclass(frozen=True)
class GroundTruthSample:
    sample_id: int
    nonce_k: int
    leaked_nonce_prefix: int


@dataclass(frozen=True)
class GroundTruth:
    private_key: int
    samples: tuple[GroundTruthSample, ...]


def generate_dataset(
    curve: Curve,
    sample_count: int = 160,
    leaked_bits: int = 8,
    seed: Optional[int] = 20260919,
) -> tuple[list[PublicSample], GroundTruth]:
    if sample_count <= 0:
        raise ValueError("sample_count must be positive.")
    # This benchmark simulates a deterministic leakage experiment; it is intentionally not a
    # cryptographic RNG, so a fixed seed must reproduce the sample set exactly.
    rng = random.Random(seed)
    n = curve.n
    private_key = rng.randrange(1, n)

    public_samples: list[PublicSample] = []
    truth_samples: list[GroundTruthSample] = []
    used_messages: set[bytes] = set()

    for sample_id in range(sample_count):
        while True:
            message = f"HNP benchmark message {sample_id} / token={rng.randrange(1 << 64):016x}".encode()
            if message not in used_messages:
                used_messages.add(message)
                break

        z = hash_message(message, n)
        while True:
            k, prefix = nonce_with_known_msb(n, leaked_bits, rng)
            try:
                r, s = ecdsa_sign(curve, private_key, z, k)
                break
            except ValueError:
                continue

        public_samples.append(
            PublicSample(
                sample_id=sample_id,
                message=message,
                z=z,
                r=r,
                s=s,
                leaked_bits=leaked_bits,
            )
        )
        truth_samples.append(
            GroundTruthSample(
                sample_id=sample_id,
                nonce_k=k,
                leaked_nonce_prefix=prefix,
            )
        )

    return public_samples, GroundTruth(private_key=private_key, samples=tuple(truth_samples))


def verify_nonce_leakage(k: int, n: int, leaked_bits: int, expected_prefix: int) -> bool:
    hidden_bits = n.bit_length() - leaked_bits
    return (k >> hidden_bits) == expected_prefix


def ecdsa_verify_with_known_nonce(curve: Curve, private_key: int, z: int, r: int, s: int, k: int) -> bool:
    n = curve.n
    if not (1 <= private_key < n and 1 <= k < n and 1 <= r < n and 1 <= s < n):
        return False
    if (s * k - z - r * private_key) % n != 0:
        return False
    R = scalar_mult(curve, k, curve.G)
    return not R.is_infinity and R.x % n == r


def public_dataset_as_dicts(public_samples: list[PublicSample]) -> list[dict]:
    return [
        {
            "sample_id": sample.sample_id,
            "message": sample.message.decode("utf-8"),
            "z": sample.z,
            "r": sample.r,
            "s": sample.s,
            "leaked_bits": sample.leaked_bits,
        }
        for sample in public_samples
    ]


def ground_truth_as_dict(ground_truth: GroundTruth) -> dict:
    return {
        "private_key": ground_truth.private_key,
        "samples": {
            str(sample.sample_id): {
                "nonce_k": sample.nonce_k,
                "leaked_nonce_prefix": sample.leaked_nonce_prefix,
            }
            for sample in ground_truth.samples
        },
    }


def save_dataset(public_samples: list[PublicSample], ground_truth: GroundTruth) -> None:
    ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)
    PUBLIC_DATASET_PATH.write_text(
        json.dumps(public_dataset_as_dicts(public_samples), indent=2), encoding="utf-8"
    )
    ORACLE_DATASET_PATH.write_text(
        json.dumps(ground_truth_as_dict(ground_truth), indent=2), encoding="utf-8"
    )


def load_public_dataset(path: Path = PUBLIC_DATASET_PATH) -> list[dict]:
    return json.loads(path.read_text(encoding="utf-8"))


def load_ground_truth(path: Path = ORACLE_DATASET_PATH) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def run_consistency_checks(curve: Curve, public_samples: list[PublicSample], ground_truth: GroundTruth) -> None:
    assert is_on_curve(curve, curve.G), "Generator is not on the curve."
    assert scalar_mult(curve, curve.n, curve.G).is_infinity, "Configured subgroup order is invalid for G."
    assert len(public_samples) == len(ground_truth.samples)

    truth_by_id = {sample.sample_id: sample for sample in ground_truth.samples}
    for public in public_samples:
        truth = truth_by_id[public.sample_id]
        assert verify_nonce_leakage(truth.nonce_k, curve.n, public.leaked_bits, truth.leaked_nonce_prefix)
        assert ecdsa_verify_with_known_nonce(curve, ground_truth.private_key, public.z, public.r, public.s, truth.nonce_k)

    print(f"[PASS] Curve validation: secp256k1 generator has order {curve.n}.")
    print(f"[PASS] Dataset validation: {len(public_samples)} signatures are internally consistent.")
    print("[PASS] Public/oracle separation: private key and complete nonces are stored outside the public dataset.")


def main() -> None:
    sample_count = 160
    leaked_bits = 12
    public_samples, ground_truth = generate_dataset(CURVE, sample_count, leaked_bits)
    run_consistency_checks(CURVE, public_samples, ground_truth)
    save_dataset(public_samples, ground_truth)
    print(f"[PASS] Public dataset written to {PUBLIC_DATASET_PATH}.")
    print(f"[PASS] Oracle dataset written to {ORACLE_DATASET_PATH}.")
    print("[PASS] Dataset generation complete.")


if __name__ == "__main__":
    main()
