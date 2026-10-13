# Arhitectura proiectului

```text
📁 RoSEF26-27/
│
├── 📁 .github/
│   └── 📁 workflows/
│       ├── 📄 benchmark.yml
│       ├── 📄 pages.yml
│       ├── 📄 reproducibility.yml
│       └── 📄 tests.yml
│
├── 📁 docs/
│   ├── 📄 ethics.md
│   ├── 📄 experimental-protocol.md
│   ├── 📄 methodology.md
│   ├── 📄 project-guide.md
│   ├── 📄 project-motivation.md
│   ├── 📄 project-overview.md
│   ├── 📄 project-summary.md
│   ├── 📄 reproducibility.md
│   └── 📄 research-question.md
│
├── 📁 informatica/
│   ├── 📄 README.md
│   ├── 📄 __init__.py
│   ├── 📁 ECDSA/
│   │   ├── 📄 Lattice_key6.py
│   │   ├── 📄 cnn_nonce_analysis5.py
│   │   ├── 📄 dataset_generator3.py
│   │   ├── 📄 diophantine_experiments7.py
│   │   ├── 📄 ecdsa_leakage_model4.py
│   │   ├── 📄 ecdsa_simulator2.py
│   │   ├── 📄 hnp_attack1.py
│   │   ├── 📄 hnp_utils.py
│   │   ├── 📄 independent_verification.py
│   │   ├── 📄 main.py
│   │   └── 📄 theory_to_experiment_bridge8.py
│   │
│   ├── 📁 artifacts/
│   │   ├── 📄 README.txt
│   │   ├── 📄 bridge_results.json
│   │   ├── 📄 cnn_predictions.json
│   │   ├── 📄 dataset_metadata.json
│   │   ├── 📄 dataset_split.json
│   │   ├── 📄 diophantine_bridge_verification.png
│   │   ├── 📄 diophantine_results.json
│   │   ├── 📄 hnp_result.json
│   │   ├── 📄 oracle_dataset.json
│   │   ├── 📄 pipeline_results.json
│   │   ├── 📄 profile_dataset.json
│   │   ├── 📄 public_dataset.json
│   │   └── 📄 trace_validation_metadata.json
│   │
│   ├── 📁 cnn/
│   │   ├── 📄 config.yaml
│   │   ├── 📄 evaluate.py
│   │   ├── 📄 export_weights.py
│   │   ├── 📄 model.py
│   │   ├── 📄 model_weights.pth
│   │   ├── 📄 train.py
│   │   └── 📄 utils.py
│   │
│   ├── 📁 dataset/
│   │   ├── 📄 README.md
│   │   ├── 📄 capture_esp32.py
│   │   ├── 📄 generate.py
│   │   └── 📁 sample/
│   │       ├── 📄 dataset_metadata.json
│   │       ├── 📄 dataset_split.json
│   │       ├── 📄 profile_dataset.json
│   │       └── 📄 public_dataset.json
│   │
│   ├── 📁 esp32/
│   │   └── 📁 firmware/
│   │       └── 📁 arduino/
│   │           ├── 📄 README.md
│   │           ├── 📄 main.ino
│   │           └── 📁 src/
│   │               ├── 📁 batch/
│   │               │   ├── 📄 batch.cpp
│   │               │   └── 📄 batch.h
│   │               ├── 📁 benchmark/
│   │               │   ├── 📄 benchmark.cpp
│   │               │   └── 📄 benchmark.h
│   │               ├── 📁 button/
│   │               │   ├── 📄 button.cpp
│   │               │   └── 📄 button.h
│   │               ├── 📁 core/
│   │               │   └── 📄 config.h
│   │               ├── 📁 crypto/
│   │               │   ├── 📄 ecdsa_exp.cpp
│   │               │   └── 📄 ecdsa_exp.h
│   │               ├── 📁 experiment/
│   │               │   ├── 📄 experiment.cpp
│   │               │   └── 📄 experiment.h
│   │               ├── 📁 ina219/
│   │               │   ├── 📄 ina219.cpp
│   │               │   └── 📄 ina219.h
│   │               ├── 📁 protocol/
│   │               │   ├── 📄 protocol.cpp
│   │               │   └── 📄 protocol.h
│   │               ├── 📁 selftest/
│   │               │   ├── 📄 selftest.cpp
│   │               │   └── 📄 selftest.h
│   │               ├── 📁 test_vectors/
│   │               │   ├── 📄 test_vectors.cpp
│   │               │   └── 📄 test_vectors.h
│   │               └── 📁 ...
│   │
│   ├── 📁 experiments/
│   │   ├── 📄 benchmark_cnn_hnp_ensemble.py
│   │   ├── 📄 benchmark_cnn_hnp_surface.py
│   │   ├── 📄 benchmark_hnp_thresholds.py
│   │   └── 📄 run_experiments.py
│   │
│   ├── 📁 lattice/
│   │   ├── 📄 README.md
│   │   ├── 📄 __init__.py
│   │   └── 📄 solver.py
│   │
│   └── 📁 simulator/
│       ├── 📄 cli.py
│       ├── 📄 config.py
│       ├── 📄 generator.py
│       └── 📄 noise.py
│
├── 📁 matematica/
│   ├── 📄 README.md
│   ├── 📄 k4-k5-verification.md
│   └── 📁 experiments/
│       ├── 📁 concurenta/
│       │   ├── 📁 figs_con/
│       │   │   └── 🖼️ ...
│       │   ├── 📄 01_concurenta_exacta.py
│       │   ├── 📄 02_near-concurenta.py
│       │   └── 📄 03_cocurenta-aprox.py
│       │
│       └── 📁 graficele_matematica/
│           ├── 📁 figs_cert/
│           │   └── 🖼️ ...
│           ├── 📄 01_plan_Vk_retea_rezonanta.py
│           ├── 📄 02_deltaM_sqrt2.py
│           ├── 📄 03_deltaM_phi.py
│           ├── 📄 04_tranzitie_rationala_zero_defect.py
│           ├── 📄 05_defect_simultan_k4.py
│           └── ...
│
├── 📁 misc/
│   ├── 🖼️ Echipa.png
│   ├── 🖼️ LEAKON.png
|   ├── 🖼️ QR-github.svg
|   ├── 🖼️ QR-leakon.svg
│   └── 🖼️ Team2.jpg
│ 
│
├── 📁 notebooks/
│   ├── 📓 01_data-generation.ipynb
│   ├── 📓 02_baseline.ipynb
│   ├── 📓 03_cnn-training.ipynb
│   ├── 📓 04_final-results.ipynb
│   └── 📓 05_trace-validation.ipynb
│
├── 📁 papers/
│   ├── 📄 Recovery_of_private_keys.pdf
│   ├── 📄 arithmetic_concurrence.pdf
│   └── 📁 source/
│       ├── 📄 README.md
│       ├── 📄 Recovery_of_private_keys.tex
│       └── 📄 arithmetic_concurrence.tex
│
├── 📁 results/
│   ├── 📄 README.md
│   ├── 📁 figures/
│   │   └── 🖼️ ...
│   ├── 📁 raw/
│   │   └── 📄 trace-validation-100.npz
│   ├── 📁 tables/
│   │   ├── 📄 cnn-hnp-baseline.csv
│   │   ├── 📄 cnn-hnp-parameter-sweep.csv
│   │   ├── 📄 cnn-hnp-parameter-sweep-summary.csv
│   │   ├── 📄 final-results.csv
│   │   ├── 📄 hnp-oracle-thresholds.csv
│   │   └── 📄 trace-validation-100.csv
│   └── 📁 validation/
│       └── 📁 ensemble-robust-100-seeds/
│           ├── 📄 rosef-comparison-100-seeds-summary.csv
│           └── 📄 rosef-comparison-100-seeds.csv
│
├── 📁 tests/
│   ├── 📄 test_benchmark_hnp_surface.py
│   ├── 📄 test_ecdsa.py
│   ├── 📄 test_hnp.py
│   ├── 📄 test_hnp_regressions.py
│   ├── 📄 test_independent_verification.py
│   ├── 📄 test_math.py
│   ├── 📄 test_model.py
│   ├── 📄 test_reference_artifacts.py
│   ├── 📄 test_simulator.py
│   └── 📄 test_smoke.py
│
├── 📁 website/
│   ├── 📄 app.js
│   ├── 📄 data.js
│   ├── 📄 index.html
│   ├── 📄 styles.css
│   ├── 📄 README.md
│   ├── 📄 DESIGN-HANDOFF.md
│   ├── 📁 paper/
│   │   └── 📄 Recovery_of_private_keys.pdf
│   └── 📁 source/
│       ├── 📁 informatica/
│       │   ├── 📄 Lattice_key6.py
│       │   ├── 📄 cnn_config.yaml
│       │   ├── 📄 cnn_model.py
│       │   ├── 📄 cnn_utils.py
│       │   ├── 📄 dataset_generator3.py
│       │   ├── 📄 ecdsa_leakage_model4.py
│       │   └── 📄 hnp_attack1.py
│       ├── 📁 results/
│       │   ├── 📄 dataset_metadata.json
│       │   ├── 📄 final-results.csv
│       │   ├── 📄 hnp_result.json
│       │   └── 📄 pipeline_results.json
│       ├── 📄 docs_experimental-protocol.md
│       ├── 📄 docs_methodology.md
│       ├── 📄 docs_project-overview.md
│       ├── 📄 docs_project-summary.md
│       └── 📄 docs_reproducibility.md
│
├── 📄 .gitignore
├── 📄 ARCHITECTURE.md
├── 📄 CITATION.cff
├── 📄 CLEANUP_CHANGELOG.md
├── 📄 LICENSE
├── 📄 Q&A.txt
├── 📄 README.md
├── 📄 requirements.txt
└── 📄 ...
```

> Structura este actualizată conform repo-ului curent și include fișierele și directoarele existente în acest proiect.
