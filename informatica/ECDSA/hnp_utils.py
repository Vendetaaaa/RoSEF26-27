from __future__ import annotations


def compute_hnp_terms(r: int, s: int, z: int, leaked_prefix: int, q: int, leaked_bits: int) -> tuple[int, int, int]:
    """Convert one ECDSA signature and a leaked MSB prefix into HNP terms (t, u, a)."""
    if not 1 <= leaked_bits < q.bit_length():
        raise ValueError("leaked_bits must be between 1 and q.bit_length()-1")
    hidden_bits = q.bit_length() - leaked_bits
    s_inv = pow(int(s), -1, q)
    t_i = (s_inv * int(r)) % q
    u_i = (s_inv * int(z)) % q
    a_i = int(leaked_prefix) << hidden_bits
    return t_i, u_i, a_i
