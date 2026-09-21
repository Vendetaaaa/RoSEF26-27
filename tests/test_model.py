from __future__ import annotations

import sys
from pathlib import Path

import torch

ROOT = Path(__file__).resolve().parents[1]
CNN_DIR = ROOT / "informatica" / "cnn"
if str(CNN_DIR) not in sys.path:
    sys.path.insert(0, str(CNN_DIR))

from model import NonceBitCNN
from utils import split_dataset


def test_cnn_forward_shape() -> None:
    model = NonceBitCNN(in_channels=2, out_channels=8, kernel_size=3)
    x = torch.randn(4, 2, 12)
    logits = model(x)
    assert logits.shape == (4, 2, 12)
    assert torch.isfinite(logits).all()


def test_dataset_split_is_disjoint_and_deterministic() -> None:
    traces = torch.arange(20 * 2 * 4, dtype=torch.float32).reshape(20, 2, 4)
    labels = torch.zeros(20, 4, dtype=torch.long)
    a = split_dataset(traces, labels, 0.75, 123)
    b = split_dataset(traces, labels, 0.75, 123)
    assert torch.equal(a[0], b[0])
    assert torch.equal(a[2], b[2])
    assert set(a[4].tolist()).isdisjoint(set(a[5].tolist()))
    assert len(a[4]) + len(a[5]) == len(labels)


def test_cnn_can_overfit_tiny_deterministic_problem() -> None:
    torch.manual_seed(7)
    model = NonceBitCNN(in_channels=1, out_channels=8, kernel_size=3)
    x = torch.zeros(4, 1, 6)
    y = torch.zeros(4, 6, dtype=torch.long)
    x[2:] = 1.0
    y[2:] = 1
    optimizer = torch.optim.Adam(model.parameters(), lr=0.05)
    loss_fn = torch.nn.CrossEntropyLoss()
    for _ in range(80):
        optimizer.zero_grad()
        loss = loss_fn(model(x), y)
        loss.backward()
        optimizer.step()
    accuracy = (model(x).argmax(dim=1) == y).float().mean().item()
    assert accuracy >= 0.95
