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
    leaked_bits = int(config["dataset"].get("leaked_bits", 12))
    if sequence_length < leaked_bits + 2:
        raise RuntimeError("TEMPORAL prefix decoding requires two trailing observations.")
    traces = traces[:, :, :sequence_length]
    labels = labels[:, :sequence_length]
    split_data = split_dataset_by_ids(traces, labels, records, split_path)
    test_x, test_y = split_data[4], split_data[5]
    expected_attack_samples = 40
    if len(split["test"]) != expected_attack_samples:
        raise RuntimeError(f"CNN/HNP evaluation requires exactly {expected_attack_samples} test samples; found {len(split['test'])}.")

    model.eval()
    with torch.no_grad():
        all_probabilities = torch.softmax(model(traces).permute(0, 2, 1), dim=2)
        all_predictions = all_probabilities.argmax(dim=2)
        test_predictions = model(test_x).argmax(dim=1)

    test_bit_accuracy = float((test_predictions == test_y).float().mean().item())
    test_prefix_matches = test_predictions[:, :leaked_bits] == test_y[:, :leaked_bits]
    test_prefix_accuracy = float(test_prefix_matches.all(dim=1).float().mean().item())
    test_exact_prefixes = int(test_prefix_matches.all(dim=1).sum().item())

    ranked_predictions = []
    for position, record in enumerate(records):
        predicted_bits = all_predictions[position]
        prefix_bits = predicted_bits[:leaked_bits]
        prefix = 0
        for bit in prefix_bits.tolist():
            prefix = (prefix << 1) | int(bit)
        chosen_probability = all_probabilities[position, :leaked_bits].gather(1, prefix_bits.unsqueeze(1)).squeeze(1)
        prefix_log_confidence = float(torch.log(chosen_probability.clamp_min(1e-12)).sum().item())
        ranked_predictions.append({
            "sample_id": int(record["sample_id"]),
            "leaked_bits": int(record["leaked_bits"]),
            "predicted_prefix": int(prefix),
            "prefix_log_confidence": prefix_log_confidence,
        })
    ranked_predictions.sort(key=lambda item: (-item["prefix_log_confidence"], item["sample_id"]))
    output = ranked_predictions[:expected_attack_samples]

    ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)
    metrics = {
        **metrics,
        "test_bit_accuracy": test_bit_accuracy,
        "test_prefix_accuracy": test_prefix_accuracy,
        "test_exact_prefixes": test_exact_prefixes,
        "test_samples": len(test_y),
        "target_prefix_bits": leaked_bits,
        "observed_sequence_length": sequence_length,
        "attack_selection_pool_size": len(records),
        "attack_selection_size": len(output),
    }
    payload = {
        "metrics": metrics,
        "split": {"name": "test", "sample_ids": split["test"]},
        "attack_selection": {
            "strategy": "top_40_by_cnn_prefix_log_confidence",
            "pool_size": len(records),
            "selected_sample_ids": [item["sample_id"] for item in output],
            "uses_reference_labels": False,
        },
        "predictions": output,
    }
    PREDICTIONS_PATH.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print(f"[PASS] CNN validation bit accuracy: {100 * metrics['validation_bit_accuracy']:.2f}%")
    print(f"[PASS] CNN validation exact-prefix accuracy ({leaked_bits} bits): {100 * metrics['validation_prefix_accuracy']:.2f}%")
    print(f"[PASS] CNN test bit accuracy: {100 * test_bit_accuracy:.2f}%")
    print(f"[PASS] CNN test exact-prefix accuracy ({leaked_bits} bits): {100 * test_prefix_accuracy:.2f}% ({test_exact_prefixes}/{len(test_y)}).")
    print(f"[INFO] Selected {len(output)} HNP inputs from {len(records)} traces by CNN confidence only.")
    print(f"[PASS] CNN prefix predictions written to {PREDICTIONS_PATH}.")
    return True


def main() -> None:
    run()


if __name__ == "__main__":
    main()
