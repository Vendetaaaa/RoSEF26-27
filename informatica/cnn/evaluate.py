from __future__ import annotations

from pathlib import Path

import torch

from model import NonceBitCNN
from utils import load_config, load_profile_dataset, split_dataset


def main() -> None:
    config = load_config()
    profile_path = Path(__file__).resolve().parent / config["dataset"]["profile_path"]
    traces, labels, _ = load_profile_dataset(profile_path)
    _, _, val_x, val_y, _, _ = split_dataset(traces, labels, config["training"]["train_split"], config["training"]["random_seed"])
    sequence_length = int(config["dataset"]["sequence_length"])
    traces = val_x[:, :, :sequence_length]
    labels = val_y[:, :sequence_length]
    model = NonceBitCNN(
        in_channels=config["model"]["in_channels"],
        out_channels=config["model"]["out_channels"],
        kernel_size=config["model"]["kernel_size"],
    )
    model.load_state_dict(torch.load(config["training"]["model_save_path"], map_location="cpu"))
    model.eval()
    with torch.no_grad():
        logits = model(traces)
        predictions = logits.argmax(dim=1)
        accuracy = float((predictions == labels).float().mean().item())
        prefix_accuracy = float((predictions == labels).all(dim=1).float().mean().item())
    print(f"[PASS] Validation bit accuracy: {accuracy * 100:.2f}%")
    print(f"[PASS] Validation prefix accuracy: {prefix_accuracy * 100:.2f}%")


if __name__ == "__main__":
    main()
