from __future__ import annotations

import torch
import torch.nn as nn


class NonceBitCNN(nn.Module):
    def __init__(self, in_channels: int = 1, out_channels: int = 16, kernel_size: int = 9, num_classes: int = 2) -> None:
        super().__init__()
        if kernel_size < 1 or kernel_size % 2 == 0:
            raise ValueError("kernel_size must be a positive odd integer.")
        if num_classes < 2:
            raise ValueError("num_classes must be at least 2.")
        padding = kernel_size // 2
        self.conv1 = nn.Conv1d(in_channels, out_channels, kernel_size, padding=padding)
        self.relu = nn.ReLU()
        self.conv2 = nn.Conv1d(out_channels, num_classes, 1)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        x = self.relu(self.conv1(x))
        return self.conv2(x)
