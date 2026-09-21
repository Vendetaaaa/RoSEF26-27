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

    sys.path.insert(0, str(CNN_DIR))
    import torch
    from model import NonceBitCNN
    from train import train_model
    from utils import load_profile_dataset

    model, config, _, metrics = train_model()
    profile_path = CNN_DIR / config["dataset"]["profile_path"]
    traces, _, records = load_profile_dataset(profile_path)
    sequence_length = int(config["dataset"]["sequence_length"])
    traces = traces[:, :, :sequence_length]
    model.eval()
    with torch.no_grad():
        predictions = model(traces).argmax(dim=1).cpu().tolist()

    output = []
    for record, predicted_bits in zip(records, predictions):
        leaked_bits = int(record["leaked_bits"])
        prefix_bits = predicted_bits[:leaked_bits]
        prefix = 0
        for bit in prefix_bits:
            prefix = (prefix << 1) | int(bit)
        output.append({
            "sample_id": record["sample_id"],
            "leaked_bits": leaked_bits,
            "predicted_prefix": prefix,
        })

    ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)
    payload = {"metrics": metrics, "predictions": output}
    PREDICTIONS_PATH.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print(f"[PASS] CNN validation bit accuracy: {100 * metrics["validation_bit_accuracy"]:.2f}%")
    print(f"[PASS] CNN validation prefix accuracy: {100 * metrics["validation_prefix_accuracy"]:.2f}%")
    print(f"[PASS] CNN prefix predictions written to {PREDICTIONS_PATH}.")
    return True


def main() -> None:
    run()


if __name__ == "__main__":
    main()
