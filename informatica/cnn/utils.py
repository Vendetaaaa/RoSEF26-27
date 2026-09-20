from __future__ import annotations

import json
from pathlib import Path

import torch
import yaml

BASE_DIR = Path(__file__).resolve().parent
CONFIG_PATH = BASE_DIR / "config.yaml"


def load_config(path: Path = CONFIG_PATH) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        config = yaml.safe_load(handle)
    config["training"]["model_save_path"] = str(BASE_DIR / config["training"]["model_save_path"])
    return config


def load_profile_dataset(path: str | Path) -> tuple[torch.Tensor, torch.Tensor, list[dict]]:
    records = json.loads(Path(path).read_text(encoding="utf-8"))
    raw = torch.tensor([record["trace"] for record in records], dtype=torch.float32)
    diff = torch.zeros_like(raw)
    diff[:, 0] = raw[:, 0]
    diff[:, 1:] = raw[:, 1:] - raw[:, :-1]
    traces = torch.stack((raw, diff), dim=1)
    labels = torch.tensor([record["bit_labels"] for record in records], dtype=torch.long)
    return traces, labels, records


def split_dataset(traces: torch.Tensor, labels: torch.Tensor, train_split: float, seed: int):
    if not 0.0 < train_split < 1.0:
        raise ValueError("train_split must be between 0 and 1.")
    generator = torch.Generator().manual_seed(seed)
    indices = torch.randperm(len(labels), generator=generator)
    train_size = int(len(labels) * train_split)
    train_idx = indices[:train_size]
    val_idx = indices[train_size:]
    return traces[train_idx], labels[train_idx], traces[val_idx], labels[val_idx], train_idx, val_idx
