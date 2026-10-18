from __future__ import annotations

import argparse
import importlib.util
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ECDSA_DIR = ROOT / "informatica" / "ECDSA"
ARTIFACTS_DIR = ROOT / "informatica" / "artifacts"


def read_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--profile-noise-sigma", type=float, default=float(os.environ.get("ROSEF_PROFILE_NOISE_SIGMA", "0.15")))
    args = parser.parse_args()
    if args.profile_noise_sigma < 0:
        parser.error("--profile-noise-sigma must be non-negative")
    entrypoint = ECDSA_DIR / "main.py"
    if not entrypoint.is_file():
        print("[FAIL] Proiectul RoSEF nu are structura așteptată.")
        return 2
    if importlib.util.find_spec("fpylll") is None:
        print("[FAIL] Lipsește fpylll. Instalați dependențele din requirements.txt și relansați comanda.")
        return 3

    environment = os.environ.copy()
    environment["OMP_NUM_THREADS"] = "1"
    environment["MKL_NUM_THREADS"] = "1"
    environment["OPENBLAS_NUM_THREADS"] = "1"
    environment["ROSEF_PROFILE_NOISE_SIGMA"] = str(args.profile_noise_sigma)
    run = subprocess.run(
        [sys.executable, str(entrypoint), "--require-fpylll"],
        cwd=ECDSA_DIR,
        env=environment,
        check=False,
    )

    result_path = ARTIFACTS_DIR / "pipeline_results.json"
    hnp_path = ARTIFACTS_DIR / "hnp_result.json"
    public_path = ARTIFACTS_DIR / "public_dataset.json"
    if not all(path.is_file() for path in (result_path, hnp_path, public_path)):
        print("[FAIL] Lipsesc rezultate necesare pentru verificarea independentă.")
        return run.returncode or 4

    pipeline = read_json(result_path)
    hnp = read_json(hnp_path)
    candidate_value = hnp.get("recovered_private_key")
    if (
        hnp.get("source") != "cnn"
        or not hnp.get("validated")
        or not hnp.get("reduction_executed")
        or isinstance(candidate_value, bool)
        or not isinstance(candidate_value, int)
    ):
        print("[FAIL] Nu există un candidat CNN→HNP validat; cheia nu va fi afișată.")
        return run.returncode or 5

    sys.path.insert(0, str(ECDSA_DIR))
    try:
        from independent_verification import verify_candidate

        public_records = read_json(public_path)
        verified = verify_candidate(candidate_value, public_records)
    except Exception as exc:
        print(f"[FAIL] Verificarea independentă nu a putut fi executată: {type(exc).__name__}.")
        return run.returncode or 6

    if not verified or not pipeline.get("hnp_public_key_verified"):
        print("[FAIL] Candidatul nu verifică cheia publică și semnăturile; cheia nu va fi afișată.")
        return run.returncode or 7

    status = "PASS" if pipeline.get("hnp") == "PASS" else "PASS_DEVELOPMENT"
    print(f"[PASS] Recuperare verificată pentru setul sintetic local ({status}).")
    print(f"[PASS] Backend LLL: {hnp.get('reduction_backend')}.")
    print(f"[PASS] Semnături verificate independent: {len(public_records)}.")
    print(f"Cheie privată de test (hex): {candidate_value:064x}")
    print(f"Rezultate: {result_path}")
    if run.returncode != 0:
        print("[WARN] Cheia a fost verificată, dar cel puțin o altă etapă a pipeline-ului a eșuat.")
    return run.returncode


if __name__ == "__main__":
    raise SystemExit(main())
