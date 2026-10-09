from __future__ import annotations

import csv
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ARTIFACTS = ROOT / "informatica" / "artifacts"
TABLES = ROOT / "results" / "tables"


def test_reference_pipeline_artifacts_are_fully_passed_and_redacted() -> None:
    pipeline = json.loads((ARTIFACTS / "pipeline_results.json").read_text(encoding="utf-8"))
    hnp = json.loads((ARTIFACTS / "hnp_result.json").read_text(encoding="utf-8"))

    assert pipeline["dataset"] == "PASS"
    assert pipeline["leakage"] == "PASS"
    assert pipeline["cnn"] == "PASS"
    assert pipeline["hnp"] == "PASS"
    assert pipeline["math"] == "PASS"
    assert pipeline["fully_executed"] is True
    assert pipeline["fpylll_available"] is True
    assert pipeline["fpylll_required"] is True
    assert pipeline["hnp_backend"] == "fpylll"
    assert pipeline["hnp_key_recovered"] is True

    assert hnp["reduction_backend"] == "fpylll"
    assert hnp["backend_policy"] == "require_fpylll"
    assert hnp["fpylll_available"] is True
    assert hnp["reduction_executed"] is True
    assert hnp["key_recovered"] is True
    assert hnp["validated"] is True
    assert str(hnp["recovered_private_key"]).startswith("<REDACTED")


def test_sweep_tables_exist_and_have_expected_size() -> None:
    raw_path = TABLES / "cnn-hnp-parameter-sweep.csv"
    summary_path = TABLES / "cnn-hnp-parameter-sweep-summary.csv"
    oracle_path = TABLES / "hnp-oracle-thresholds.csv"

    assert raw_path.is_file()
    assert summary_path.is_file()
    assert oracle_path.is_file()

    with raw_path.open(newline="", encoding="utf-8-sig") as f:
        raw = list(csv.DictReader(f))
    with summary_path.open(newline="", encoding="utf-8-sig") as f:
        summary = list(csv.DictReader(f))
    with oracle_path.open(newline="", encoding="utf-8-sig") as f:
        oracle = list(csv.DictReader(f))

    assert len(raw) == 2000
    assert len(summary) == 20
    assert len(oracle) == 400


def test_sweep_uses_fpylll_where_reduction_runs_and_redacts_key_values() -> None:
    with (TABLES / "cnn-hnp-parameter-sweep.csv").open(newline="", encoding="utf-8-sig") as f:
        raw = list(csv.DictReader(f))
    assert all(row["hnp_backend"] in {"", "fpylll"} for row in raw)

    with (TABLES / "hnp-oracle-thresholds.csv").open(newline="", encoding="utf-8-sig") as f:
        oracle = list(csv.DictReader(f))
    assert "recovered_private_key" in oracle[0]
    assert all(not row["recovered_private_key"] or row["recovered_private_key"] == "<REDACTED_PRIVATE_KEY>" for row in oracle)


def test_website_reference_records_match_project_artifacts() -> None:
    root_pipeline = json.loads((ARTIFACTS / "pipeline_results.json").read_text(encoding="utf-8"))
    site_pipeline = json.loads((ROOT / "website" / "source" / "pipeline_results.json").read_text(encoding="utf-8"))
    root_hnp = json.loads((ARTIFACTS / "hnp_result.json").read_text(encoding="utf-8"))
    site_hnp = json.loads((ROOT / "website" / "source" / "hnp_result.json").read_text(encoding="utf-8"))

    assert site_pipeline == root_pipeline
    assert site_hnp == root_hnp
    assert site_pipeline["hnp"] == "PASS"
    assert site_hnp["reduction_backend"] == "fpylll"

    site_data = (ROOT / "website" / "data.js").read_text(encoding="utf-8")
    assert '"hnp": "PASS"' in site_data
    assert '"hnp_backend": "fpylll"' in site_data
    assert 'factStatus:"PASS_DEVELOPMENT"' not in site_data
    assert "snapshot-ul arhivei, execuția HNP folosea backendul SymPy" not in site_data


def test_generated_metadata_uses_repository_relative_paths() -> None:
    metadata = json.loads((ARTIFACTS / "trace_validation_metadata.json").read_text(encoding="utf-8"))
    for value in metadata["outputs"].values():
        assert not value.startswith("/")
        assert not (len(value) > 2 and value[1:3] == ":\\")
        assert (ROOT / value).is_file()



def test_public_oracle_key_remains_redacted() -> None:
    oracle = json.loads((ARTIFACTS / "oracle_dataset.json").read_text(encoding="utf-8"))
    private_key = str(oracle.get("private_key", "")).strip()
    assert private_key
    assert not private_key.isdecimal()


def test_website_source_mirrors_match_canonical_files() -> None:
    mirrors = {
        "informatica/artifacts/dataset_metadata.json": "website/source/dataset_metadata.json",
        "informatica/artifacts/pipeline_results.json": "website/source/pipeline_results.json",
        "informatica/artifacts/hnp_result.json": "website/source/hnp_result.json",
        "results/tables/final-results.csv": "website/source/final-results.csv",
        "docs/experimental-protocol.md": "website/source/docs_experimental-protocol.md",
        "docs/methodology.md": "website/source/docs_methodology.md",
        "docs/project-overview.md": "website/source/docs_project-overview.md",
        "docs/project-summary.md": "website/source/docs_project-summary.md",
        "docs/reproducibility.md": "website/source/docs_reproducibility.md",
    }
    for canonical_rel, mirror_rel in mirrors.items():
        canonical = ROOT / canonical_rel
        mirror = ROOT / mirror_rel
        assert mirror.is_file()
        canonical_text = canonical.read_text(encoding="utf-8")
        mirror_text = mirror.read_text(encoding="utf-8")
        assert (
        mirror_text.replace("\r\n", "\n")
        == canonical_text.replace("\r\n", "\n")
        )


def test_website_reference_config_matches_canonical_dataset() -> None:
    metadata = json.loads((ARTIFACTS / "dataset_metadata.json").read_text(encoding="utf-8"))
    site_metadata = json.loads((ROOT / "website/source/dataset_metadata.json").read_text(encoding="utf-8"))
    assert site_metadata == metadata
    site_data = (ROOT / "website/data.js").read_text(encoding="utf-8")
    assert f'"seed": "{metadata["seed"]}"' in site_data
    assert '"backend_policy": "require_fpylll"' in site_data
    assert '"reduction_backend": "fpylll"' in site_data


def test_final_results_notebook_handles_redacted_ground_truth() -> None:
    notebook = json.loads((ROOT / "notebooks/04_final-results.ipynb").read_text(encoding="utf-8"))
    cells = ["".join(cell.get("source", [])) for cell in notebook.get("cells", [])]
    source = "\n".join(cells)
    assert "ground_truth_key_available" in source
    assert "Ground-truth private key is redacted" in source
    assert '"NOT EXECUTED"' in source
    assert 'private_key = int(oracle["private_key"])' not in source



def test_site_sweep_matches_summary_csv() -> None:
    site_text = (ROOT / "website/data.js").read_text(encoding="utf-8")
    start_marker = '"paperSweep": '
    start = site_text.index(start_marker) + len(start_marker)
    end_marker = '\n],\n    "traceSampleId"'
    end = site_text.index(end_marker, start) + 2
    site_sweep = json.loads(site_text[start:end])

    with (TABLES / "cnn-hnp-parameter-sweep-summary.csv").open(newline="", encoding="utf-8-sig") as f:
        rows = list(csv.DictReader(f))

    expected = []
    for row in rows:
        expected.append({
            "ell": int(row["leaked_bits"]),
            "sigma": float(row["sigma"]),
            "bit": round(float(row["mean_cnn_test_bit_accuracy"]) * 100, 2),
            "prefix": round(float(row["mean_cnn_test_prefix_accuracy"]) * 100, 2),
            "oracle": f'{int(row["oracle_hnp_recoveries"])}/{int(row["runs"])}',
            "cnn": f'{int(row["cnn_hnp_recoveries"])}/{int(row["runs"])}',
        })
    assert site_sweep == expected



def test_data_generation_notebook_uses_canonical_split_and_warns_about_oracle_redaction() -> None:
    notebook = json.loads((ROOT / "notebooks/01_data-generation.ipynb").read_text(encoding="utf-8"))
    cells = ["".join(cell.get("source", [])) for cell in notebook.get("cells", [])]
    source = "\n".join(cells)

    assert "train_fraction=0.60" in source
    assert "validation_fraction=0.15" in source
    assert '{"train": 96, "validation": 24, "test": 40}' in source
    assert "<REDACTED_PRIVATE_KEY>" in source
    assert not any(cell.get("outputs") for cell in notebook.get("cells", []) if cell.get("cell_type") == "code")


def test_website_fallback_and_paper_pdf_match_canonical_reference() -> None:
    html = (ROOT / "website/index.html").read_text(encoding="utf-8").lower()
    app = (ROOT / "website/app.js").read_text(encoding="utf-8").lower()
    assert "archive snapshot" not in html
    assert "recorded reference run" in html
    assert "pass_development" not in app

    canonical_pdf = ROOT / "papers/Recovery_of_private_keys.pdf"
    website_pdf = ROOT / "website/paper/Recovery_of_private_keys.pdf"
    assert canonical_pdf.is_file()
    assert website_pdf.is_file()
    assert canonical_pdf.read_bytes() == website_pdf.read_bytes()
