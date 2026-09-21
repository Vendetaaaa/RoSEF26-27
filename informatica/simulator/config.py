from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class SimulationConfig:
    """Configuration for the deterministic synthetic side-channel trace simulator."""

    leaked_bits: int = 12
    trace_length: int = 256
    noise_sigma: float = 0.15
    alpha: float = 1.0
    leakage_model: str = "HW"
    seed: int = 20260919
    normalize_trace: bool = False

    def validate(self) -> None:
        nbits = 256
        if not 1 <= self.leaked_bits < nbits:
            raise ValueError("leaked_bits must be in [1, 255].")
        if not self.leaked_bits <= self.trace_length <= nbits:
            raise ValueError("trace_length must be between leaked_bits and 256.")
        if self.noise_sigma < 0:
            raise ValueError("noise_sigma must be non-negative.")
        if self.alpha <= 0:
            raise ValueError("alpha must be positive.")
        if self.leakage_model.upper() not in {"HW", "HD"}:
            raise ValueError("leakage_model must be HW or HD.")
