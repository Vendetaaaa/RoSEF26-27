from __future__ import annotations

import logging
import math
from dataclasses import dataclass
from typing import List, Optional

try:
    from fpylll import IntegerMatrix, LLL
    FPYLLL_AVAILABLE = True
except ImportError:
    FPYLLL_AVAILABLE = False

from dataset_generator3 import CURVE

logger = logging.getLogger(__name__)


@dataclass(frozen=True)
class HNPConfig:
    q: int = CURVE.n
    nbits: int = CURVE.n.bit_length()
    leaked_bits: int = 8
    num_samples: int = 40
    embedding_factor: int = 0
    delta: float = 0.75

    @property
    def bound(self) -> int:
        return 1 << (self.nbits - self.leaked_bits)

    @property
    def embedding(self) -> int:
        if self.embedding_factor > 0:
            return self.embedding_factor
        return self.q * self.bound


class HNPLatticeSolver:
    def __init__(self, config: HNPConfig) -> None:
        self.config = config

    def build_basis_matrix(self, t_list: List[int], u_list: List[int], a_list: List[int]) -> List[List[int]]:
        m = len(t_list)
        if m != len(u_list) or m != len(a_list):
            raise ValueError("HNP sample arrays must have equal length.")
        if m < 2:
            raise ValueError("At least two HNP samples are required.")
        if not 1 <= self.config.leaked_bits < self.config.nbits:
            raise ValueError("leaked_bits must be between 1 and nbits-1.")
        if self.config.q != CURVE.n:
            raise ValueError("The configured HNP modulus must match the shared secp256k1 subgroup order.")

        q = self.config.q
        B = self.config.bound
        M = self.config.embedding
        dim = m + 2
        matrix = [[0] * dim for _ in range(dim)]

        for i in range(m):
            matrix[i][i] = q * q

        for i in range(m):
            matrix[m][i] = q * (t_list[i] % q)
        matrix[m][m] = B

        for i in range(m):
            c_i = (a_list[i] - u_list[i]) % q
            matrix[m + 1][i] = q * c_i
        matrix[m + 1][m + 1] = M
        return matrix

    def expected_short_vector(self, alpha: int, t_list: List[int], u_list: List[int], a_list: List[int]) -> List[int]:
        q = self.config.q
        B = self.config.bound
        M = self.config.embedding
        deltas = [((t * alpha + u - a) % q) for t, u, a in zip(t_list, u_list, a_list)]
        if not all(0 <= delta < B for delta in deltas):
            raise ValueError("Provided alpha does not satisfy the supplied leakage bounds.")
        return [q * delta for delta in deltas] + [alpha * B, -M]

    def verify_expected_vector(self, alpha: int, t_list: List[int], u_list: List[int], a_list: List[int]) -> bool:
        q = self.config.q
        B = self.config.bound
        M = self.config.embedding
        matrix = self.build_basis_matrix(t_list, u_list, a_list)
        vector = self.expected_short_vector(alpha, t_list, u_list, a_list)
        for index, (t_i, u_i, a_i) in enumerate(zip(t_list, u_list, a_list)):
            delta_i = (t_i * alpha + u_i - a_i) % q
            c_i = (a_i - u_i) % q
            coefficient = (q * delta_i - alpha * q * (t_i % q) + q * c_i)
            if coefficient % (q * q) != 0:
                return False
            expected_row_value = coefficient // (q * q) * matrix[index][index] + alpha * matrix[-2][index] - matrix[-1][index]
            if expected_row_value != vector[index]:
                return False
        return alpha * B == vector[-2] and -M == vector[-1]

    def recover_from_reduced_basis(self, reduced_matrix: List[List[int]], t_list: List[int], u_list: List[int], a_list: List[int]) -> Optional[int]:
        q = self.config.q
        B = self.config.bound
        M = self.config.embedding
        m = len(t_list)
        for row in reduced_matrix:
            if abs(row[m + 1]) != M or row[m] % B != 0:
                continue
            for sign in (1, -1):
                candidate = (sign * (row[m] // B)) % q
                if candidate and self.validate_candidate(candidate, t_list, u_list, a_list):
                    return candidate
        return None

    def validate_candidate(self, candidate: int, t_list: List[int], u_list: List[int], a_list: List[int]) -> bool:
        if not 1 <= candidate < self.config.q:
            return False
        B = self.config.bound
        return all(0 <= ((t * candidate + u - a) % self.config.q) < B for t, u, a in zip(t_list, u_list, a_list))

    def reduce(self, matrix: List[List[int]]) -> List[List[int]]:
        if FPYLLL_AVAILABLE:
            L = IntegerMatrix(len(matrix), len(matrix))
            for r, row in enumerate(matrix):
                for c, value in enumerate(row):
                    L[r, c] = int(value)
            LLL.reduction(L, delta=self.config.delta)
            return [[int(L[r, c]) for c in range(len(matrix))] for r in range(len(matrix))]

        try:
            from sympy import Matrix, Rational
        except ImportError as exc:
            raise RuntimeError("Install fpylll or sympy to run lattice reduction.") from exc
        reduced = Matrix(matrix).lll(delta=Rational(str(self.config.delta)))
        return [[int(reduced[r, c]) for c in range(reduced.cols)] for r in range(reduced.rows)]

    def solve(self, t_list: List[int], u_list: List[int], a_list: List[int]) -> Optional[int]:
        matrix = self.build_basis_matrix(t_list, u_list, a_list)
        reduced = self.reduce(matrix)
        return self.recover_from_reduced_basis(reduced, t_list, u_list, a_list)
