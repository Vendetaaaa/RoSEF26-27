from __future__ import annotations

import importlib.util
import json
import sys
from pathlib import Path

ARTIFACTS_DIR = Path(__file__).resolve().parents[1] / "artifacts"
CNN_DIR = Path(__file__).resolve().parents[1] / "cnn"
PREDICTIONS_PATH = ARTIFACTS_DIR / "cnn_predictions.json"


def torch_available() -> bool:
    return importlib.util.find_spec("torch") is not None


def run() -> bool:
    if not torch_available():
        print("[SKIP] PyTorch is not installed; CNN nonce decoding was not executed.")
        return False

    if str(CNN_DIR) not in sys.path:
        sys.path.insert(0, str(CNN_DIR))
    import torch
    from train import train_model
    from utils import load_dataset_split, load_profile_dataset, split_dataset_by_ids

    model, config, _, metrics = train_model()
    profile_path = CNN_DIR / config["dataset"]["profile_path"]
    traces, labels, records = load_profile_dataset(profile_path)
    split_path = Path(config["dataset"]["split_path"])
    if not split_path.is_absolute():
        split_path = (CNN_DIR / split_path).resolve()
    split = load_dataset_split(split_path)
    sequence_length = int(config["dataset"]["sequence_length"])
    traces = traces[:, :, :sequence_length]
    labels = labels[:, :sequence_length]

    split_data = split_dataset_by_ids(traces, labels, records, split_path)
    test_x, test_y = split_data[4], split_data[5]
    expected_attack_samples = 40
    if len(split["test"]) != expected_attack_samples:
        raise RuntimeError(f"CNN/HNP coupling requires exactly {expected_attack_samples} test samples; found {len(split['test'])}.")

    model.eval()
    with torch.no_grad():
        all_predictions = model(traces).argmax(dim=1)
        test_predictions = model(test_x).argmax(dim=1)

    test_bit_accuracy = float((test_predictions == test_y).float().mean().item())
    test_prefix_accuracy = float((test_predictions == test_y).all(dim=1).float().mean().item())
    test_exact_prefixes = int((test_predictions == test_y).all(dim=1).sum().item())

    output = []
    predictions = all_predictions.cpu().tolist()
    record_by_id = {
        int(record["sample_id"]): (record, predicted_bits)
        for record, predicted_bits in zip(records, predictions)
    }
    for sample_id in split["test"]:
        record, predicted_bits = record_by_id[int(sample_id)]
        leaked_bits = int(record["leaked_bits"])
        prefix = 0
        for bit in predicted_bits[:leaked_bits]:
            prefix = (prefix << 1) | int(bit)
        output.append({
            "sample_id": record["sample_id"],
            "leaked_bits": leaked_bits,
            "predicted_prefix": prefix,
        })

    ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)
    metrics = {
        **metrics,
        "test_bit_accuracy": test_bit_accuracy,
        "test_prefix_accuracy": test_prefix_accuracy,
        "test_exact_prefixes": test_exact_prefixes,
        "test_samples": len(test_y),
    }
    payload = {
        "metrics": metrics,
        "split": {"name": "test", "sample_ids": split["test"]},
        "predictions": output,
    }
    PREDICTIONS_PATH.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print(f"[PASS] CNN validation bit accuracy: {100 * metrics['validation_bit_accuracy']:.2f}%")
    print(f"[PASS] CNN validation prefix accuracy: {100 * metrics['validation_prefix_accuracy']:.2f}%")
    print(f"[PASS] CNN test bit accuracy: {100 * test_bit_accuracy:.2f}%")
    print(f"[PASS] CNN test prefix accuracy: {100 * test_prefix_accuracy:.2f}% ({test_exact_prefixes}/{len(test_y)}).")
    print(f"[PASS] CNN prefix predictions written to {PREDICTIONS_PATH}.")
    return True


def main() -> None:
    run()


if __name__ == "__main__":
    main()
