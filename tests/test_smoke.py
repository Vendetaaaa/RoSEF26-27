from __future__ import annotations

import importlib


def test_core_modules_import() -> None:
    modules = [
        "informatica.ECDSA.dataset_generator3",
        "informatica.ECDSA.ecdsa_simulator2",
        "informatica.ECDSA.ecdsa_leakage_model4",
        "informatica.cnn.model",
        "informatica.cnn.utils",
    ]
    for name in modules:
        assert importlib.import_module(name) is not None
