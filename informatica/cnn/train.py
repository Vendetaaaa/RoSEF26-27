from __future__ import annotations

import random
from pathlib import Path

import numpy as np
import torch
import torch.nn as nn
from torch.utils.data import DataLoader, TensorDataset

from model import NonceBitCNN
from utils import load_config, load_profile_dataset, split_dataset


def set_seed(seed: int) -> None:
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(seed)


def train_model() -> tuple[NonceBitCNN, dict, tuple[torch.Tensor, torch.Tensor], dict]:
    config = load_config()
    set_seed(config["training"]["random_seed"])
    configured_profile = Path(config["dataset"]["profile_path"])
    profile_path = configured_profile if configured_profile.is_absolute() else (Path(__file__).resolve().parent / configured_profile).resolve()
    traces, labels, _ = load_profile_dataset(profile_path)
    sequence_length = int(config["dataset"]["sequence_length"])
    traces = traces[:, :, :sequence_length]
    labels = labels[:, :sequence_length]
    train_x, train_y, val_x, val_y, _, _ = split_dataset(traces, labels, config["training"]["train_split"], config["training"]["random_seed"])
    train_loader = DataLoader(TensorDataset(train_x, train_y), batch_size=config["training"]["batch_size"], shuffle=True)
    val_loader = DataLoader(TensorDataset(val_x, val_y), batch_size=config["training"]["batch_size"], shuffle=False)

    model = NonceBitCNN(
        in_channels=config["model"]["in_channels"],
        out_channels=config["model"]["out_channels"],
        kernel_size=config["model"]["kernel_size"],
        num_classes=config["model"]["num_classes"],
    )
    criterion = nn.CrossEntropyLoss()
    optimizer = torch.optim.AdamW(
        model.parameters(),
        lr=config["training"]["learning_rate"],
        weight_decay=config["training"]["weight_decay"],
    )

    for epoch in range(1, config["training"]["epochs"] + 1):
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

        print(f"Epoch {epoch:02d}: train_loss={train_loss/train_total:.4f} train_acc={100*train_correct/train_total:.2f}% val_loss={val_loss/val_total:.4f} val_acc={100*val_correct/val_total:.2f}%")

    save_path = Path(config["training"]["model_save_path"])
    save_path.parent.mkdir(parents=True, exist_ok=True)
    torch.save(model.state_dict(), save_path)
    with torch.no_grad():
        val_logits = model(val_x)
        val_predictions = val_logits.argmax(dim=1)
        bit_accuracy = float((val_predictions == val_y).float().mean().item())
        prefix_accuracy = float((val_predictions == val_y).all(dim=1).float().mean().item())
    metrics = {
        "validation_bit_accuracy": bit_accuracy,
        "validation_prefix_accuracy": prefix_accuracy,
        "validation_samples": int(len(val_y)),
    }
    print(f"[PASS] Model saved to {save_path}.")
    print(f"[PASS] Validation bit accuracy: {100 * bit_accuracy:.2f}%")
    print(f"[PASS] Validation prefix accuracy: {100 * prefix_accuracy:.2f}%")
    return model, config, (val_x, val_y), metrics


if __name__ == "__main__":
    train_model()
