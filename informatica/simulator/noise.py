from __future__ import annotations

import random
from typing import Sequence


def add_gaussian_noise(values: Sequence[float], sigma: float, rng: random.Random) -> list[float]:
    if sigma < 0:
        raise ValueError("sigma must be non-negative.")
    return [float(value) + rng.gauss(0.0, sigma) for value in values]


def normalize(values: Sequence[float]) -> list[float]:
    if not values:
        return []
    mean = sum(values) / len(values)
    variance = sum((value - mean) ** 2 for value in values) / len(values)
    if variance == 0:
        return [0.0 for _ in values]
    scale = variance ** 0.5
    return [(value - mean) / scale for value in values]
