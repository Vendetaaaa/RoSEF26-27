from __future__ import annotations

import json
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


def augment_temporal_training(
    train_x: torch.Tensor,
    train_y: torch.Tensor,
    config: dict,
    profile_path: Path,
) -> tuple[torch.Tensor, torch.Tensor, int]:
    copies = int(config["training"].get("synthetic_augmentations", 0))
    metadata_path = profile_path.with_name("profile_metadata.json")
    if copies <= 0 or not metadata_path.is_file():
        return train_x, train_y, 0
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    if metadata.get("synthetic") is not True or str(metadata.get("leakage_model", "")).upper() != "TEMPORAL":
        return train_x, train_y, 0
    sigma = float(metadata["profile_noise_sigma"])
    if sigma < 0:
        raise ValueError("profile_noise_sigma cannot be negative.")
    bits = train_y.to(dtype=torch.float32)
    previous = torch.nn.functional.pad(bits[:, :-1], (1, 0))
    previous_two = torch.nn.functional.pad(bits[:, :-2], (2, 0))
    alpha = float(metadata.get("leakage_alpha", 1.0))
    clean = alpha * (0.2 * bits + 0.6 * previous + 0.2 * previous_two)
    generator = torch.Generator(device="cpu").manual_seed(int(config["training"]["random_seed"]) + 1709)
    repeated_clean = clean.repeat_interleave(copies, dim=0)
    noise = torch.randn(repeated_clean.shape, generator=generator, dtype=repeated_clean.dtype) * sigma
    raw = repeated_clean + noise
    diff = torch.zeros_like(raw)
    diff[:, 0] = raw[:, 0]
    diff[:, 1:] = raw[:, 1:] - raw[:, :-1]
    augmented_x = torch.stack((raw, diff), dim=1)
    augmented_y = train_y.repeat_interleave(copies, dim=0)
    return torch.cat((train_x, augmented_x), dim=0), torch.cat((train_y, augmented_y), dim=0), copies


def make_temporal_sequences(
    sample_count: int,
    sequence_length: int,
    sigma: float,
    alpha: float,
    seed: int,
) -> tuple[torch.Tensor, torch.Tensor]:
    if sample_count < 1 or sequence_length < 3 or sigma < 0:
        raise ValueError("Invalid synthetic temporal dataset parameters.")
    generator = torch.Generator(device="cpu").manual_seed(seed)
    labels = torch.randint(0, 2, (sample_count, sequence_length), generator=generator, dtype=torch.long)
    bits = labels.to(dtype=torch.float32)
    previous = torch.nn.functional.pad(bits[:, :-1], (1, 0))
    previous_two = torch.nn.functional.pad(bits[:, :-2], (2, 0))
    clean = alpha * (0.2 * bits + 0.6 * previous + 0.2 * previous_two)
    raw = clean + torch.randn(clean.shape, generator=generator, dtype=clean.dtype) * sigma
    diff = torch.zeros_like(raw)
    diff[:, 0] = raw[:, 0]
    diff[:, 1:] = raw[:, 1:] - raw[:, :-1]
    return torch.stack((raw, diff), dim=1), labels


def fit_model(
    train_x: torch.Tensor,
    train_y: torch.Tensor,
    val_x: torch.Tensor,
    val_y: torch.Tensor,
    config: dict,
    prefix_bits: int,
) -> tuple[NonceBitCNN, dict]:
    set_seed(int(config["training"]["random_seed"]))
    torch.set_num_threads(int(config["training"].get("torch_num_threads", 1)))
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

    best_val_prefix_accuracy = -1.0
    best_state = None
    epochs = int(config["training"]["epochs"])
    for epoch in range(1, epochs + 1):
        model.train()
        train_loss = 0.0
        train_correct = 0
        train_total = 0
        for batch_x, batch_y in train_loader:
            optimizer.zero_grad(set_to_none=True)
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
        prefix_correct = 0
        prefix_total = 0
        with torch.no_grad():
            for batch_x, batch_y in val_loader:
                logits = model(batch_x)
                loss = criterion(logits, batch_y)
                predicted = logits.argmax(dim=1)
                val_loss += loss.item() * batch_y.numel()
                val_correct += int((predicted == batch_y).sum().item())
                val_total += batch_y.numel()
                prefix_correct += int((predicted[:, :prefix_bits] == batch_y[:, :prefix_bits]).all(dim=1).sum().item())
                prefix_total += len(batch_y)

        val_bit_accuracy = val_correct / val_total
        val_prefix_accuracy = prefix_correct / prefix_total
        if val_prefix_accuracy > best_val_prefix_accuracy:
            best_val_prefix_accuracy = val_prefix_accuracy
            best_state = {key: value.detach().clone() for key, value in model.state_dict().items()}

        print(
            f"Epoch {epoch:03d}: train_loss={train_loss/train_total:.4f} "
            f"train_acc={100*train_correct/train_total:.2f}% "
            f"val_loss={val_loss/val_total:.4f} "
            f"val_acc={100*val_bit_accuracy:.2f}% "
            f"prefix_acc={100*val_prefix_accuracy:.2f}%"
        )

    if best_state is not None:
        model.load_state_dict(best_state)

    model.eval()
    with torch.no_grad():
        val_predictions = torch.cat([model(batch_x).argmax(dim=1) for batch_x, _ in val_loader])
        bit_accuracy = float((val_predictions == val_y).float().mean().item())
        prefix_accuracy = float((val_predictions[:, :prefix_bits] == val_y[:, :prefix_bits]).all(dim=1).float().mean().item())

    return model, {
        "validation_bit_accuracy": bit_accuracy,
        "validation_prefix_accuracy": prefix_accuracy,
        "validation_samples": int(len(val_y)),
        "validation_prefix_bits": int(prefix_bits),
        "epochs": epochs,
    }


def train_model() -> tuple[NonceBitCNN, dict, tuple[torch.Tensor, torch.Tensor], dict]:
    config = load_config()
    configured_profile = Path(config["dataset"]["profile_path"])
    profile_path = configured_profile if configured_profile.is_absolute() else (Path(__file__).resolve().parent / configured_profile).resolve()
    traces, labels, records = load_profile_dataset(profile_path)
    sequence_length = int(config["dataset"]["sequence_length"])
    prefix_bits = int(config["dataset"].get("leaked_bits", 12))
    if sequence_length < prefix_bits + 2:
        raise ValueError("Temporal prefix decoding requires at least two observations beyond the leaked prefix.")
    traces = traces[:, :, :sequence_length]
    labels = labels[:, :sequence_length]
    split_path = Path(config["dataset"]["split_path"])
    if not split_path.is_absolute():
        split_path = (Path(__file__).resolve().parent / split_path).resolve()
    split_data = split_dataset_by_ids(traces, labels, records, split_path)
    train_x_real, train_y_real, val_x_real, val_y_real, _test_x, _test_y, train_idx, val_idx, test_idx, _split = split_data

    metadata_path = profile_path.with_name("profile_metadata.json")
    metadata = json.loads(metadata_path.read_text(encoding="utf-8")) if metadata_path.is_file() else {}
    temporal_synthetic = metadata.get("synthetic") is True and str(metadata.get("leakage_model", "")).upper() == "TEMPORAL"
    if temporal_synthetic:
        sigma = float(metadata["profile_noise_sigma"])
        alpha = float(metadata.get("leakage_alpha", 1.0))
        seed = int(config["training"]["random_seed"])
        train_count = int(config["training"].get("synthetic_random_training_samples", 16384))
        val_count = int(config["training"].get("synthetic_random_validation_samples", 4096))
        train_x, train_y = make_temporal_sequences(train_count, sequence_length, sigma, alpha, seed + 1)
        val_x, val_y = make_temporal_sequences(val_count, sequence_length, sigma, alpha, seed + 2)
        augmentation_copies = 0
        print(f"[INFO] Independent synthetic TEMPORAL training sequences: {train_count}; validation sequences: {val_count}; sigma={sigma:g}; observed_length={sequence_length}; target_prefix_bits={prefix_bits}.")
    else:
        train_x, train_y, augmentation_copies = augment_temporal_training(train_x_real, train_y_real, config, profile_path)
        val_x, val_y = val_x_real, val_y_real
        print(f"[INFO] Profile-based training; augmentation replicas per training sample: {augmentation_copies}.")

    model, metrics = fit_model(train_x, train_y, val_x, val_y, config, prefix_bits)
    metrics["synthetic_temporal_augmentation_copies"] = augmentation_copies
    metrics["synthetic_temporal_training_samples"] = len(train_y) if temporal_synthetic else 0
    metrics["synthetic_temporal_validation_samples"] = len(val_y) if temporal_synthetic else 0
    metrics["temporal_observed_sequence_length"] = sequence_length
    metrics["target_prefix_bits"] = prefix_bits
    metrics["test_samples"] = int(len(test_idx))
    save_path = Path(config["training"]["model_save_path"])
    save_path.parent.mkdir(parents=True, exist_ok=True)
    torch.save(model.state_dict(), save_path)
    print(f"[PASS] Split sizes: train={len(train_idx)}, validation={len(val_idx)}, test={len(test_idx)}.")
    print(f"[PASS] Model saved to {save_path}.")
    print(f"[PASS] Validation bit accuracy: {100 * metrics['validation_bit_accuracy']:.2f}%")
    print(f"[PASS] Validation exact-prefix accuracy ({prefix_bits} bits): {100 * metrics['validation_prefix_accuracy']:.2f}%")
    return model, config, (val_x, val_y), metrics


if __name__ == "__main__":
    train_model()
