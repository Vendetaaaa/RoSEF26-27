from __future__ import annotations

import random
from pathlib import Path

import numpy as np
import torch
import torch.nn as nn
from torch.utils.data import DataLoader, TensorDataset

from model import NonceBitCNN
from utils import load_config, load_profile_dataset, split_dataset_by_ids


def set_seed(seed: int) -> None:
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(seed)


def build_model(config: dict) -> NonceBitCNN:
    return NonceBitCNN(
        in_channels=config["model"]["in_channels"],
        out_channels=config["model"]["out_channels"],
        kernel_size=config["model"]["kernel_size"],
        num_classes=config["model"]["num_classes"],
    )


def fit_model(
    train_x: torch.Tensor,
    train_y: torch.Tensor,
    val_x: torch.Tensor,
    val_y: torch.Tensor,
    config: dict,
) -> tuple[NonceBitCNN, dict]:
    """Train one nonce-bit CNN on an explicit split and return validation metrics."""
    set_seed(int(config["training"]["random_seed"]))
    model = build_model(config)
    criterion = nn.CrossEntropyLoss()
    optimizer = torch.optim.AdamW(
        model.parameters(),
        lr=float(config["training"]["learning_rate"]),
        weight_decay=float(config["training"]["weight_decay"]),
    )
    train_loader = DataLoader(
        TensorDataset(train_x, train_y),
        batch_size=int(config["training"]["batch_size"]),
        shuffle=True,
    )
    val_loader = DataLoader(
        TensorDataset(val_x, val_y),
        batch_size=int(config["training"]["batch_size"]),
        shuffle=False,
    )

    best_val_bit_accuracy = -1.0
    best_state = None
    epochs = int(config["training"]["epochs"])
    for epoch in range(1, epochs + 1):
        model.train()
        train_loss = 0.0
        train_correct = 0
        train_total = 0
        for batch_x, batch_y in train_loader:
            optimizer.zero_grad()
            logits = model(batch_x)
            loss = criterion(logits, batch_y)
            loss.backward()
            optimizer.step()
            train_loss += loss.item() * batch_y.numel()
            train_correct += int((logits.argmax(dim=1) == batch_y).sum().item())
            train_total += batch_y.numel()

        model.eval()
        val_loss = 0.0
        val_correct = 0
        val_total = 0
        with torch.no_grad():
            for batch_x, batch_y in val_loader:
                logits = model(batch_x)
                loss = criterion(logits, batch_y)
                val_loss += loss.item() * batch_y.numel()
                val_correct += int((logits.argmax(dim=1) == batch_y).sum().item())
                val_total += batch_y.numel()

        val_bit_accuracy = val_correct / val_total
        if val_bit_accuracy > best_val_bit_accuracy:
            best_val_bit_accuracy = val_bit_accuracy
            best_state = {key: value.detach().clone() for key, value in model.state_dict().items()}

        print(
            f"Epoch {epoch:03d}: train_loss={train_loss/train_total:.4f} "
            f"train_acc={100*train_correct/train_total:.2f}% "
            f"val_loss={val_loss/val_total:.4f} val_acc={100*val_bit_accuracy:.2f}%"
        )

    if best_state is not None:
        model.load_state_dict(best_state)

    model.eval()
    with torch.no_grad():
        val_predictions = model(val_x).argmax(dim=1)
        bit_accuracy = float((val_predictions == val_y).float().mean().item())
        prefix_accuracy = float((val_predictions == val_y).all(dim=1).float().mean().item())

    return model, {
        "validation_bit_accuracy": bit_accuracy,
        "validation_prefix_accuracy": prefix_accuracy,
        "validation_samples": int(len(val_y)),
        "epochs": epochs,
    }


def train_model() -> tuple[NonceBitCNN, dict, tuple[torch.Tensor, torch.Tensor], dict]:
    config = load_config()
    configured_profile = Path(config["dataset"]["profile_path"])
    profile_path = (
        configured_profile
        if configured_profile.is_absolute()
        else (Path(__file__).resolve().parent / configured_profile).resolve()
    )
    traces, labels, records = load_profile_dataset(profile_path)
    sequence_length = int(config["dataset"]["sequence_length"])
    traces = traces[:, :, :sequence_length]
    labels = labels[:, :sequence_length]
    split_path = Path(config["dataset"]["split_path"])
    if not split_path.is_absolute():
        split_path = (Path(__file__).resolve().parent / split_path).resolve()
    (
        train_x, train_y, val_x, val_y, _test_x, _test_y,
        train_idx, val_idx, test_idx, _split,
    ) = split_dataset_by_ids(traces, labels, records, split_path)

    model, metrics = fit_model(train_x, train_y, val_x, val_y, config)
    save_path = Path(config["training"]["model_save_path"])
    save_path.parent.mkdir(parents=True, exist_ok=True)
    torch.save(model.state_dict(), save_path)
    metrics["test_samples"] = int(len(test_idx))

    print(f"[PASS] Split sizes: train={len(train_idx)}, validation={len(val_idx)}, test={len(test_idx)}.")
    print(f"[PASS] Model saved to {save_path}.")
    print(f"[PASS] Validation bit accuracy: {100 * metrics['validation_bit_accuracy']:.2f}%")
    print(f"[PASS] Validation prefix accuracy: {100 * metrics['validation_prefix_accuracy']:.2f}%")
    return model, config, (val_x, val_y), metrics


if __name__ == "__main__":
    train_model()
