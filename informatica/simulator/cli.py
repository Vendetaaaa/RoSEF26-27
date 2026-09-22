from __future__ import annotations

import argparse
import json
from dataclasses import asdict

from config import SimulationConfig
from generator import generate_simulated_dataset


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate synthetic ECDSA side-channel traces.")
    parser.add_argument("--samples", type=int, default=160)
    parser.add_argument("--leaked-bits", type=int, default=12)
    parser.add_argument("--trace-length", type=int, default=256)
    parser.add_argument("--sigma", type=float, default=0.15)
    parser.add_argument("--model", choices=("HW", "HD"), default="HW")
    parser.add_argument("--seed", type=int, default=20260919)
    parser.add_argument("--normalize", action="store_true")
    args = parser.parse_args()
    config = SimulationConfig(
        leaked_bits=args.leaked_bits,
        trace_length=args.trace_length,
        noise_sigma=args.sigma,
        leakage_model=args.model,
        seed=args.seed,
        normalize_trace=args.normalize,
    )
    public, truth, traces = generate_simulated_dataset(args.samples, config)
    print(json.dumps({
        "config": asdict(config),
        "samples": len(public),
        "trace_length": len(traces[0].trace) if traces else 0,
        "label_length": len(traces[0].bit_labels) if traces else 0,
        "private_key_hidden": bool(truth.private_key),
    }, indent=2))


if __name__ == "__main__":
    main()
