from __future__ import annotations

import argparse
import json
import random
from pathlib import Path
import sys
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
ECDSA_DIR = ROOT / "informatica" / "ECDSA"
SIM_DIR = ROOT / "informatica" / "simulator"
for path in (ECDSA_DIR, SIM_DIR):
    if str(path) not in sys.path:
        sys.path.insert(0, str(path))

from dataset_generator3 import (  # noqa: E402
    CURVE,
    generate_dataset,
    public_dataset_as_dicts,
    ground_truth_as_dict,
)
from ecdsa_leakage_model4 import (  # noqa: E402
    add_gaussian_noise,
    generate_clean_leakage,
    integer_to_bits,
)

ARTIFACTS = ROOT / "informatica" / "artifacts"
PUBLIC_PATH = ARTIFACTS / "public_dataset.json"
ORACLE_PATH = ARTIFACTS / "oracle_dataset.json"
PROFILE_PATH = ARTIFACTS / "profile_dataset.json"
METADATA_PATH = ARTIFACTS / "dataset_metadata.json"
SPLIT_PATH = ARTIFACTS / "dataset_split.json"


def _validate_public(records: list[dict[str, Any]], leaked_bits: int) -> None:
    if not records:
        raise ValueError("Dataset is empty.")
    ids = [int(r["sample_id"]) for r in records]
    if ids != list(range(len(records))):
        raise ValueError("sample_id values must be consecutive starting at 0.")
    for record in records:
        if int(record["leaked_bits"]) != leaked_bits:
            raise ValueError("Inconsistent leaked_bits in public dataset.")
        for field in ("z", "r", "s"):
            value = int(record[field])
            if not 0 <= value < CURVE.n:
                raise ValueError(f"{field} is outside the curve order.")


def _build_profile(
    public_records: list[dict[str, Any]],
    oracle: dict[str, Any],
    *,
    noise_sigma: float,
    seed: int,
    mode: str,
) -> list[dict[str, Any]]:
    rng = random.Random(seed)
    width = CURVE.n.bit_length()
    profile: list[dict[str, Any]] = []
    samples = oracle["samples"]

    for record in public_records:
        truth = samples[str(record["sample_id"])]
        nonce = int(truth["nonce_k"])
        clean = generate_clean_leakage(nonce, width, mode=mode, alpha=1.0)
        noisy = add_gaussian_noise(clean, noise_sigma, rng)
        bits = integer_to_bits(nonce, width)
        leaked = int(record["leaked_bits"])
        profile.append(
            {
                "sample_id": int(record["sample_id"]),
                "z": int(record["z"]),
                "r": int(record["r"]),
                "s": int(record["s"]),
                "leaked_bits": leaked,
                "trace": noisy,
                "bit_labels": bits[:leaked],
                "trace_source": "synthetic",
            }
        )
    return profile


def _split_ids(count: int, train_fraction: float, validation_fraction: float, seed: int) -> dict[str, list[int]]:
    if not 0.0 < train_fraction < 1.0 or not 0.0 <= validation_fraction < 1.0:
        raise ValueError("Invalid split fractions.")
    if train_fraction + validation_fraction >= 1.0:
        raise ValueError("train_fraction + validation_fraction must be < 1.")
    ids = list(range(count))
    random.Random(seed).shuffle(ids)
    train_end = int(count * train_fraction)
    val_end = train_end + int(count * validation_fraction)
    return {"train": ids[:train_end], "validation": ids[train_end:val_end], "test": ids[val_end:]}


def generate(
    *,
    sample_count: int = 160,
    leaked_bits: int = 12,
    noise_sigma: float = 0.15,
    seed: int = 20260919,
    leakage_mode: str = "HW",
    train_fraction: float = 0.70,
    validation_fraction: float = 0.15,
) -> dict[str, Any]:
    if sample_count < 16:
        raise ValueError("sample_count must be at least 16 for train/validation/test experiments.")
    if not 1 <= leaked_bits <= CURVE.n.bit_length():
        raise ValueError("leaked_bits must be in [1, 256].")
    if noise_sigma < 0:
        raise ValueError("noise_sigma must be non-negative.")

    public, truth = generate_dataset(CURVE, sample_count, leaked_bits, seed)
    public_dict = public_dataset_as_dicts(public)
    oracle_dict = ground_truth_as_dict(truth)
    _validate_public(public_dict, leaked_bits)

    profile = _build_profile(
        public_dict,
        oracle_dict,
        noise_sigma=noise_sigma,
        seed=seed,
        mode=leakage_mode,
    )
    split = _split_ids(sample_count, train_fraction, validation_fraction, seed + 1)

    ARTIFACTS.mkdir(parents=True, exist_ok=True)
    PUBLIC_PATH.write_text(json.dumps(public_dict, indent=2), encoding="utf-8")
    ORACLE_PATH.write_text(json.dumps(oracle_dict, indent=2), encoding="utf-8")
    PROFILE_PATH.write_text(json.dumps(profile, indent=2), encoding="utf-8")
    SPLIT_PATH.write_text(json.dumps(split, indent=2), encoding="utf-8")

    metadata = {
        "format_version": 2,
        "curve": "secp256k1",
        "sample_count": sample_count,
        "leaked_bits": leaked_bits,
        "trace_length": CURVE.n.bit_length(),
        "noise_sigma": noise_sigma,
        "leakage_model": leakage_mode.upper(),
        "seed": seed,
        "synthetic": True,
        "public_file": PUBLIC_PATH.name,
        "oracle_file": ORACLE_PATH.name,
        "profile_file": PROFILE_PATH.name,
        "split_file": SPLIT_PATH.name,
    }
    METADATA_PATH.write_text(json.dumps(metadata, indent=2), encoding="utf-8")

    return metadata


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate the reproducible ECDSA/leakage dataset.")
    parser.add_argument("--samples", type=int, default=160)
    parser.add_argument("--leaked-bits", type=int, default=12)
    parser.add_argument("--noise", type=float, default=0.15)
    parser.add_argument("--seed", type=int, default=20260919)
    parser.add_argument("--mode", choices=("HW", "HD"), default="HW")
    args = parser.parse_args()

    metadata = generate(
        sample_count=args.samples,
        leaked_bits=args.leaked_bits,
        noise_sigma=args.noise,
        seed=args.seed,
        leakage_mode=args.mode,
    )
    print(f"[PASS] Generated {metadata['sample_count']} samples.")
    print(f"[PASS] Public dataset: {PUBLIC_PATH}")
    print(f"[PASS] Oracle dataset: {ORACLE_PATH}")
    print(f"[PASS] CNN profile dataset: {PROFILE_PATH}")
    print(f"[PASS] Split metadata: {SPLIT_PATH}")


if __name__ == "__main__":
    main()
