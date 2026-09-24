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
    """Return a deterministic train/validation split for unit tests and small experiments."""
    if not 0.0 < train_split < 1.0:
        raise ValueError("train_split must be between 0 and 1.")
    generator = torch.Generator().manual_seed(seed)
    indices = torch.randperm(len(labels), generator=generator)
    train_size = int(len(labels) * train_split)
    train_idx = indices[:train_size]
    val_idx = indices[train_size:]
    return traces[train_idx], labels[train_idx], traces[val_idx], labels[val_idx], train_idx, val_idx


def load_dataset_split(path: str | Path) -> dict[str, list[int]]:
    payload = json.loads(Path(path).read_text(encoding="utf-8"))
    required = {"train", "validation", "test"}
    if set(payload) != required:
        raise ValueError(f"Dataset split must contain exactly {sorted(required)}.")
    split = {name: [int(sample_id) for sample_id in payload[name]] for name in required}
    sets = {name: set(ids) for name, ids in split.items()}
    if any(not ids for ids in split.values()):
        raise ValueError("Every dataset split must contain at least one sample.")
    if sets["train"] & sets["validation"] or sets["train"] & sets["test"] or sets["validation"] & sets["test"]:
        raise ValueError("Dataset splits must be disjoint.")
    if len(set().union(*sets.values())) != sum(len(ids) for ids in split.values()):
        raise ValueError("Dataset split contains duplicate sample IDs.")
    return split


def split_dataset_by_ids(
    traces: torch.Tensor, labels: torch.Tensor, records: list[dict], split_path: str | Path
):
    split = load_dataset_split(split_path)
    id_to_position = {int(record["sample_id"]): index for index, record in enumerate(records)}
    if len(id_to_position) != len(records):
        raise ValueError("Profile dataset contains duplicate sample IDs.")
    missing = [sample_id for ids in split.values() for sample_id in ids if sample_id not in id_to_position]
    if missing:
        raise ValueError(f"Dataset split references unknown sample IDs: {missing[:5]}")

    def select(name: str) -> torch.Tensor:
        indices = torch.tensor([id_to_position[sample_id] for sample_id in split[name]], dtype=torch.long)
        return indices

    train_idx = select("train")
    val_idx = select("validation")
    test_idx = select("test")
    return (
        traces[train_idx], labels[train_idx],
        traces[val_idx], labels[val_idx],
        traces[test_idx], labels[test_idx],
        train_idx, val_idx, test_idx, split,
    )
