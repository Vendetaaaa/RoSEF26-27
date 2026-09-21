from __future__ import annotations

import sys
from pathlib import Path
from typing import Iterable

ECDSA_DIR = Path(__file__).resolve().parents[1] / "ECDSA"
if str(ECDSA_DIR) not in sys.path:
    sys.path.insert(0, str(ECDSA_DIR))

from Lattice_key6 import HNPConfig, HNPLatticeSolver  # noqa: E402


def build_hnp_inputs(public_samples: Iterable[object], leaked_prefixes: dict[int, int], q: int, leaked_bits: int) -> tuple[list[int], list[int], list[int]]:
    """Convert public ECDSA samples + leaked MSB prefixes to HNP inputs."""
    hidden_bits = q.bit_length() - leaked_bits
    if not 1 <= leaked_bits < q.bit_length():
        raise ValueError("leaked_bits must be between 1 and q.bit_length()-1")

    t_list: list[int] = []
    u_list: list[int] = []
    a_list: list[int] = []
    for record in public_samples:
        sample_id = int(record.sample_id)
        if sample_id not in leaked_prefixes:
            raise KeyError(f"Missing leaked prefix for sample {sample_id}")
        inv_s = pow(int(record.s), -1, q)
        t_list.append((inv_s * int(record.r)) % q)
        u_list.append((inv_s * int(record.z)) % q)
        a_list.append(int(leaked_prefixes[sample_id]) << hidden_bits)
    return t_list, u_list, a_list
