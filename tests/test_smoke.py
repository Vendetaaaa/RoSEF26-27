from __future__ import annotations

import importlib
import sys
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
ECDSA_DIR = PROJECT_ROOT / "informatica" / "ECDSA"
for path in (PROJECT_ROOT, ECDSA_DIR):
    if str(path) not in sys.path:
        sys.path.insert(0, str(path))


def test_core_modules_import() -> None:
    modules = [
        "dataset_generator3",
        "ecdsa_simulator2",
        "ecdsa_leakage_model4",
        "informatica.cnn.model",
        "informatica.cnn.utils",
    ]
    for name in modules:
        assert importlib.import_module(name) is not None
