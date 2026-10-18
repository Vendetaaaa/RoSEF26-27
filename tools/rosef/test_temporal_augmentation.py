from __future__ import annotations

import json
import sys
from pathlib import Path

import torch

ROOT = Path(__file__).resolve().parents[2]
CNN_DIR = ROOT / "informatica" / "cnn"
if str(CNN_DIR) not in sys.path:
    sys.path.insert(0, str(CNN_DIR))

from train import augment_temporal_training, make_temporal_sequences


def test_temporal_augmentation_generates_consistent_synthetic_traces(tmp_path: Path) -> None:
    profile_path = tmp_path / "profile_dataset.json"
    (tmp_path / "profile_metadata.json").write_text(
        json.dumps({"synthetic": True, "leakage_model": "TEMPORAL", "profile_noise_sigma": 0.0, "leakage_alpha": 1.0}),
        encoding="utf-8",
    )
    train_x = torch.zeros((1, 2, 4), dtype=torch.float32)
    train_y = torch.tensor([[1, 0, 1, 1]], dtype=torch.long)
    config = {"training": {"synthetic_augmentations": 2, "random_seed": 7}}

    augmented_x, augmented_y, copies = augment_temporal_training(train_x, train_y, config, profile_path)

    assert copies == 2
    assert augmented_x.shape == (3, 2, 4)
    assert augmented_y.shape == (3, 4)
    assert torch.equal(augmented_y[1], train_y[0])
    assert torch.allclose(augmented_x[1, 0], torch.tensor([0.2, 0.6, 0.4, 0.8]))


def test_temporal_augmentation_is_disabled_for_other_or_non_synthetic_models(tmp_path: Path) -> None:
    profile_path = tmp_path / "profile_dataset.json"
    (tmp_path / "profile_metadata.json").write_text(
        json.dumps({"synthetic": True, "leakage_model": "HW", "profile_noise_sigma": 0.15}),
        encoding="utf-8",
    )
    train_x = torch.zeros((2, 2, 4), dtype=torch.float32)
    train_y = torch.zeros((2, 4), dtype=torch.long)
    config = {"training": {"synthetic_augmentations": 2, "random_seed": 7}}

    augmented_x, augmented_y, copies = augment_temporal_training(train_x, train_y, config, profile_path)

    assert copies == 0
    assert augmented_x is train_x
    assert augmented_y is train_y


def test_temporal_sequence_generator_is_deterministic_and_keeps_two_channels() -> None:
    first_x, first_y = make_temporal_sequences(8, 14, 0.15, 1.0, 123)
    second_x, second_y = make_temporal_sequences(8, 14, 0.15, 1.0, 123)

    assert first_x.shape == (8, 2, 14)
    assert first_y.shape == (8, 14)
    assert torch.equal(first_y, second_y)
    assert torch.equal(first_x, second_x)
    raw = first_x[:, 0, :]
    diff = first_x[:, 1, :]
    assert torch.allclose(diff[:, 0], raw[:, 0])
    assert torch.allclose(diff[:, 1:], raw[:, 1:] - raw[:, :-1])
