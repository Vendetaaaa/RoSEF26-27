from __future__ import annotations

import torch
import torch.nn as nn


class TemporalResidualBlock(nn.Module):
    def __init__(self, channels: int, dilation: int) -> None:
        super().__init__()
        self.conv1 = nn.Conv1d(channels, channels, 3, padding=dilation, dilation=dilation)
        self.conv2 = nn.Conv1d(channels, channels, 3, padding=dilation, dilation=dilation)
        self.activation = nn.GELU()

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        residual = self.conv2(self.activation(self.conv1(self.activation(x))))
        return self.activation(x + residual)


class NonceBitCNN(nn.Module):
    def __init__(self, in_channels: int = 2, out_channels: int = 32, kernel_size: int = 5, num_classes: int = 2) -> None:
        super().__init__()
        if kernel_size < 1 or kernel_size % 2 == 0:
            raise ValueError("kernel_size must be a positive odd integer.")
        if num_classes < 2:
            raise ValueError("num_classes must be at least 2.")
        self.stem = nn.Conv1d(in_channels, out_channels, kernel_size, padding=kernel_size // 2)
        self.activation = nn.GELU()
        self.blocks = nn.Sequential(
            TemporalResidualBlock(out_channels, 1),
            TemporalResidualBlock(out_channels, 2),
            TemporalResidualBlock(out_channels, 1),
        )
        self.head = nn.Conv1d(out_channels, num_classes, 1)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        x = self.activation(self.stem(x))
        x = self.blocks(x)
        return self.head(x)
