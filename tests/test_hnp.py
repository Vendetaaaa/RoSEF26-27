import json
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[1]
ECDSA_DIR = ROOT / "informatica" / "ECDSA"
if str(ECDSA_DIR) not in sys.path:
    sys.path.insert(0, str(ECDSA_DIR))

import dataset_generator3
import main as pipeline_main


def test_cnn_config_tracks_leaked_bits():
    config = yaml.safe_load((ROOT / "informatica" / "cnn" / "config.yaml").read_text(encoding="utf-8"))
    assert config["dataset"]["leaked_bits"] == 12
    assert config["dataset"]["sequence_length"] == config["dataset"]["leaked_bits"]


def test_dataset_generation_is_seeded():
    first = dataset_generator3.generate_dataset(dataset_generator3.CURVE, sample_count=16, leaked_bits=12, seed=12345)
    second = dataset_generator3.generate_dataset(dataset_generator3.CURVE, sample_count=16, leaked_bits=12, seed=12345)
    assert first[1].private_key == second[1].private_key
    assert [sample.sample_id for sample in first[0]] == [sample.sample_id for sample in second[0]]
    assert [sample.leaked_nonce_prefix for sample in first[1].samples] == [sample.leaked_nonce_prefix for sample in second[1].samples]


def test_hnp_pipeline_requires_actual_key_recovery():
    status = pipeline_main.evaluate_hnp_status(recovered_key=None, relation_ok=True, private_key=123)
    assert status == "VALIDATED"
    assert pipeline_main.is_hnp_pipeline_pass({"hnp": "VALIDATED"}) is False
