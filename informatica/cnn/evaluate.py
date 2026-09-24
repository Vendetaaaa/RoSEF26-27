from __future__ import annotations

from pathlib import Path

import torch

from model import NonceBitCNN
from utils import load_config, load_profile_dataset, split_dataset_by_ids


def main() -> None:
    config = load_config()
    configured_profile = Path(config["dataset"]["profile_path"])
    profile_path = configured_profile if configured_profile.is_absolute() else (Path(__file__).resolve().parent / configured_profile).resolve()
    traces, labels, records = load_profile_dataset(profile_path)
    split_path = Path(config["dataset"]["split_path"])
    if not split_path.is_absolute():
        split_path = (Path(__file__).resolve().parent / split_path).resolve()
    traces = traces[:, :, : int(config["dataset"]["sequence_length"])]
    labels = labels[:, : int(config["dataset"]["sequence_length"])]
    train_x, train_y, val_x, val_y, test_x, test_y, *_ = split_dataset_by_ids(traces, labels, records, split_path)
    model = NonceBitCNN(
        in_channels=config["model"]["in_channels"],
        out_channels=config["model"]["out_channels"],
        kernel_size=config["model"]["kernel_size"],
        num_classes=config["model"]["num_classes"],
    )
    model.load_state_dict(torch.load(config["training"]["model_save_path"], map_location="cpu"))
    model.eval()
    with torch.no_grad():
        val_predictions = model(val_x).argmax(dim=1)
        test_predictions = model(test_x).argmax(dim=1)
    val_accuracy = float((val_predictions == val_y).float().mean().item())
    val_prefix = float((val_predictions == val_y).all(dim=1).float().mean().item())
    test_accuracy = float((test_predictions == test_y).float().mean().item())
    test_prefix = float((test_predictions == test_y).all(dim=1).float().mean().item())
    print(f"[PASS] Validation bit accuracy: {100 * val_accuracy:.2f}%")
    print(f"[PASS] Validation prefix accuracy: {100 * val_prefix:.2f}%")
    print(f"[PASS] Test bit accuracy: {100 * test_accuracy:.2f}%")
    print(f"[PASS] Test prefix accuracy: {100 * test_prefix:.2f}% ({int(test_prefix * len(test_y))}/{len(test_y)}).")


if __name__ == "__main__":
    main()
