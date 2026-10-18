from __future__ import annotations

import argparse
import csv
import datetime as dt
import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ARTIFACTS = ROOT / "informatica" / "artifacts"
CURVE_N = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141
REQUIRED = (
    "tools/rosef/run_rosef.py",
    "requirements.txt",
    "informatica/ECDSA/main.py",
    "informatica/ECDSA/independent_verification.py",
    "informatica/cnn/config.yaml",
    "tests",
)


def read_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def save_log(path: Path, stdout: str, stderr: str) -> None:
    text = stdout
    if stderr:
        text += ("\n" if text else "") + stderr
    path.write_text(text, encoding="utf-8", errors="replace")


def check_structure() -> tuple[str, str]:
    missing = [name for name in REQUIRED if not (ROOT / name).exists()]
    return ("FAIL", "Lipsesc: " + ", ".join(missing)) if missing else ("PASS", f"Structura proiectului este prezentă ({len(REQUIRED)} repere).")


def check_syntax() -> tuple[str, str]:
    excluded = {".git", ".venv", "venv", "node_modules", "__pycache__", ".pytest_cache", "batch_reports"}
    count = 0
    failures = []
    for path in ROOT.rglob("*.py"):
        if any(part in excluded for part in path.parts):
            continue
        try:
            compile(path.read_bytes(), str(path), "exec")
            count += 1
        except Exception as exc:
            failures.append(f"{path.relative_to(ROOT)} ({type(exc).__name__})")
    if failures:
        return "FAIL", "Fișiere Python invalide: " + ", ".join(failures[:20])
    return "PASS", f"Sintaxă validă pentru {count} fișiere Python; fără fișiere .pyc generate."


def check_reference_artifacts() -> tuple[str, str]:
    try:
        pipeline = read_json(ARTIFACTS / "pipeline_results.json")
        hnp = read_json(ARTIFACTS / "hnp_result.json")
        oracle = read_json(ARTIFACTS / "oracle_dataset.json")
        public = read_json(ARTIFACTS / "public_dataset.json")
    except Exception as exc:
        return "FAIL", f"Artefactele de referință nu pot fi citite ({type(exc).__name__})."

    if not isinstance(public, list) or len(public) != 160:
        return "FAIL", "Setul public trebuie să conțină exact 160 de semnături sintetice."
    seen = set()
    forbidden = {"private_key", "nonce_k", "leaked_nonce_prefix"}
    for record in public:
        if not isinstance(record, dict) or forbidden.intersection(record):
            return "FAIL", "Setul public conține câmpuri care trebuie să rămână în afara lui."
        try:
            sample_id = int(record["sample_id"])
            r = int(record["r"])
            s = int(record["s"])
            int(record["z"])
            leaked_bits = int(record["leaked_bits"])
        except (KeyError, TypeError, ValueError):
            return "FAIL", "O semnătură publică are o structură invalidă."
        if sample_id in seen or not 1 <= r < CURVE_N or not 1 <= s < CURVE_N or leaked_bits != 12:
            return "FAIL", "Setul public conține identificatori, semnături sau parametri invalizi."
        seen.add(sample_id)

    private_key = str(oracle.get("private_key", ""))
    recovered = str(hnp.get("recovered_private_key", ""))
    if "REDACTED" not in private_key.upper() or "REDACTED" not in recovered.upper():
        return "FAIL", "Artefactele de referință nu redacționează valorile cheilor sintetice."
    expected = (
        pipeline.get("dataset") == "PASS"
        and pipeline.get("leakage") == "PASS"
        and pipeline.get("cnn") == "PASS"
        and pipeline.get("hnp") == "PASS"
        and pipeline.get("math") == "PASS"
        and pipeline.get("fully_executed") is True
        and pipeline.get("hnp_backend") == "fpylll"
        and hnp.get("reduction_backend") == "fpylll"
        and hnp.get("validated") is True
        and hnp.get("reduction_executed") is True
    )
    if not expected:
        return "FAIL", "Artefactele de referință nu indică un pipeline complet validat cu fpylll."
    return "PASS", "160 de semnături verificate structural; cheile din artefactele de referință sunt redacționate."


def run_unit_tests(log_path: Path) -> tuple[str, str, float]:
    start = time.monotonic()
    result = subprocess.run(
        [sys.executable, "-m", "pytest", "-q"],
        cwd=ROOT,
        capture_output=True,
        text=True,
        errors="replace",
        check=False,
    )
    save_log(log_path, result.stdout, result.stderr)
    duration = time.monotonic() - start
    if result.returncode == 0:
        last_lines = [line.strip() for line in result.stdout.splitlines() if line.strip()]
        detail = last_lines[-1] if last_lines else "Suita pytest a trecut."
        return "PASS", detail, duration
    return "FAIL", f"pytest a returnat codul {result.returncode}; vezi {log_path.name}.", duration


def copy_ignore(directory: str, names: list[str]) -> set[str]:
    ignored = {".git", ".venv", "venv", "node_modules", "__pycache__", ".pytest_cache", "batch_reports"}
    return {name for name in names if name in ignored or name.endswith(".pyc")}


def run_isolated_pipeline(log_path: Path, profile_noise_sigma: float) -> tuple[str, str, float]:
    start = time.monotonic()
    if importlib.util.find_spec("fpylll") is None:
        log_path.write_text("Pipeline neexecutat: dependența fpylll lipsește din mediul Python curent.\n", encoding="utf-8")
        return "BLOCKED", "Lipsește fpylll; instalați dependențele din requirements.txt.", time.monotonic() - start

    with tempfile.TemporaryDirectory(prefix="rosef-batch-") as temp:
        workspace = Path(temp) / ROOT.name
        shutil.copytree(ROOT, workspace, ignore=copy_ignore)
        entrypoint = workspace / "informatica" / "ECDSA" / "main.py"
        environment = os.environ.copy()
        environment["OMP_NUM_THREADS"] = "1"
        environment["MKL_NUM_THREADS"] = "1"
        environment["OPENBLAS_NUM_THREADS"] = "1"
        environment["ROSEF_PROFILE_NOISE_SIGMA"] = str(profile_noise_sigma)
        result = subprocess.run(
            [sys.executable, str(entrypoint), "--require-fpylll"],
            cwd=entrypoint.parent,
            env=environment,
            capture_output=True,
            text=True,
            errors="replace",
            check=False,
        )
        save_log(log_path, result.stdout, result.stderr)

        artifacts = workspace / "informatica" / "artifacts"
        required_paths = [artifacts / name for name in ("pipeline_results.json", "hnp_result.json", "public_dataset.json")]
        if not all(path.is_file() for path in required_paths):
            return "FAIL", f"Pipeline-ul nu a creat toate artefactele cerute; vezi {log_path.name}.", time.monotonic() - start
        try:
            pipeline = read_json(required_paths[0])
            hnp = read_json(required_paths[1])
            public_records = read_json(required_paths[2])
            candidate = hnp.get("recovered_private_key")
            if isinstance(candidate, bool) or not isinstance(candidate, int):
                return "FAIL", "Candidatul CNN→HNP nu este un întreg valid; candidatul nu a fost raportat.", time.monotonic() - start
            ecdsa_dir = workspace / "informatica" / "ECDSA"
            sys.path.insert(0, str(ecdsa_dir))
            try:
                spec = importlib.util.spec_from_file_location(
                    "rosef_batch_independent_verification",
                    ecdsa_dir / "independent_verification.py",
                )
                if spec is None or spec.loader is None:
                    return "FAIL", "Verificatorul independent nu poate fi încărcat.", time.monotonic() - start
                module = importlib.util.module_from_spec(spec)
                spec.loader.exec_module(module)
                independent_ok = module.verify_candidate(candidate, public_records)
            finally:
                if sys.path and sys.path[0] == str(ecdsa_dir):
                    sys.path.pop(0)
        except Exception as exc:
            return "FAIL", f"Verificarea independentă a eșuat ({type(exc).__name__}); vezi {log_path.name}.", time.monotonic() - start

        pipeline_ok = (
            result.returncode == 0
            and pipeline.get("dataset") == "PASS"
            and pipeline.get("leakage") == "PASS"
            and pipeline.get("cnn") == "PASS"
            and pipeline.get("hnp") == "PASS"
            and pipeline.get("math") == "PASS"
            and pipeline.get("fully_executed") is True
            and pipeline.get("hnp_backend") == "fpylll"
            and pipeline.get("hnp_key_recovered") is True
            and pipeline.get("hnp_public_key_verified") is True
            and hnp.get("source") == "cnn"
            and hnp.get("validated") is True
            and hnp.get("reduction_executed") is True
            and hnp.get("reduction_backend") == "fpylll"
            and hnp.get("key_recovered") is True
            and independent_ok is True
            and len(public_records) == 160
        )
        if not pipeline_ok:
            return "FAIL", f"Pipeline-ul sintetic sau verificarea independentă nu a trecut; vezi {log_path.name}.", time.monotonic() - start
        return "PASS", f"Pipeline sintetic complet; model TEMPORAL, profile_noise_sigma={profile_noise_sigma:g}; candidat CNN→HNP verificat independent contra a 160 de semnături; cheia nu este inclusă în raport.", time.monotonic() - start


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--skip-pipeline", action="store_true")
    parser.add_argument("--report-dir", type=Path)
    parser.add_argument("--profile-noise-sigma", type=float, default=0.15)
    args = parser.parse_args()
    if args.profile_noise_sigma < 0:
        parser.error("--profile-noise-sigma must be non-negative")

    stamp = dt.datetime.now().strftime("%Y%m%d-%H%M%S-%f")
    report_dir = args.report_dir.resolve() if args.report_dir else ROOT / "results" / "batch_reports" / f"run-{stamp}"
    report_dir.mkdir(parents=True, exist_ok=True)
    records = []

    for name, checker in (
        ("project_structure", check_structure),
        ("python_syntax", check_syntax),
        ("reference_artifacts", check_reference_artifacts),
    ):
        started = time.monotonic()
        status, detail = checker()
        records.append({"check": name, "status": status, "duration_seconds": round(time.monotonic() - started, 3), "detail": detail, "log": ""})
        print(f"[{status}] {name}: {detail}")

    status, detail, duration = run_unit_tests(report_dir / "pytest.log")
    records.append({"check": "unit_tests", "status": status, "duration_seconds": round(duration, 3), "detail": detail, "log": "pytest.log"})
    print(f"[{status}] unit_tests: {detail}")

    if args.skip_pipeline:
        records.append({"check": "synthetic_pipeline", "status": "SKIP", "duration_seconds": 0.0, "detail": "Omit de --skip-pipeline.", "log": ""})
        print("[SKIP] synthetic_pipeline: omis la cerere.")
    else:
        status, detail, duration = run_isolated_pipeline(report_dir / "synthetic_pipeline.log", args.profile_noise_sigma)
        records.append({"check": "synthetic_pipeline", "status": status, "duration_seconds": round(duration, 3), "detail": detail, "log": "synthetic_pipeline.log"})
        print(f"[{status}] synthetic_pipeline: {detail}")

    statuses = {record["status"] for record in records}
    overall = "FAIL" if "FAIL" in statuses else ("BLOCKED" if "BLOCKED" in statuses else "PASS")
    report = {
        "created_at": dt.datetime.now().astimezone().isoformat(timespec="seconds"),
        "overall_status": overall,
        "root": str(ROOT),
        "profile_noise_sigma": args.profile_noise_sigma,
        "leakage_model": "TEMPORAL",
        "checks": records,
        "private_key_included": False,
    }
    (report_dir / "report.json").write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    with (report_dir / "report.csv").open("w", newline="", encoding="utf-8-sig") as handle:
        writer = csv.DictWriter(handle, fieldnames=("check", "status", "duration_seconds", "detail", "log"))
        writer.writeheader()
        writer.writerows(records)

    print(f"Overall: {overall}")
    print(f"Raport JSON: {report_dir / 'report.json'}")
    print(f"Raport CSV: {report_dir / 'report.csv'}")
    if overall == "FAIL":
        return 1
    if overall == "BLOCKED":
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
