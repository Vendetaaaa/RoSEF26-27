from __future__ import annotations

import torch

from model import NonceBitCNN
from utils import load_config


def main() -> None:
    config = load_config()
    model = NonceBitCNN(
        in_channels=config["model"]["in_channels"],
        out_channels=config["model"]["out_channels"],
        kernel_size=config["model"]["kernel_size"],
    )
    model.load_state_dict(torch.load(config["training"]["model_save_path"], map_location="cpu"))
    model.eval()
    print("conv1_weights =", model.conv1.weight.detach().numpy().tolist())
    print("conv1_biases =", model.conv1.bias.detach().numpy().tolist())
    print("conv2_weights =", model.conv2.weight.detach().numpy().tolist())
    print("conv2_biases =", model.conv2.bias.detach().numpy().tolist())


if __name__ == "__main__":
    main()
