from __future__ import annotations

import torch
import torch.nn as nn


class NonceBitCNN(nn.Module):
    def __init__(self, in_channels: int = 1, out_channels: int = 16, kernel_size: int = 9) -> None:
        super().__init__()
        if kernel_size < 1 or kernel_size % 2 == 0:
            raise ValueError("kernel_size must be a positive odd integer.")
        padding = kernel_size // 2
        self.conv1 = nn.Conv1d(in_channels, out_channels, kernel_size, padding=padding)
        self.relu = nn.ReLU()
        self.conv2 = nn.Conv1d(out_channels, 2, 1)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        x = self.relu(self.conv1(x))
        return self.conv2(x)
