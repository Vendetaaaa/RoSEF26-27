# Arhitectura proiectului:

```
📁 RoSEF26-27/
│
├── 📁 .github/
│   ├── 📁 workflows/
│   │   ├── 📄 benchmark.yml
│   │   ├── 📄 reproducibility.yml
│   │   └── 📄 tests.yml
│   │
│   └── 📄 pull_request_template.md
│
├── 📁 docs/
│   ├── 📄 ethics.md
│   ├── 📄 experimental-protocol.md
│   ├── 📄 methodology.md
│   ├── 📄 project-overview.md
│   ├── 📄 reproducibility.md
│   ├── 📄 research-question.md
│   │
│   └── 📁 papers/
│       ├── 📄 Recovery_of_private_keys.pdf
│       └── 📄 Recovery_of_private_keys.tex
│
├── 📁 informatica/
│   │
│   ├── 📁 ECDSA/
│   │   ├── 📄 Lattice_key6.py
│   │   ├── 📄 cnn_nonce_analysis5.py
│   │   ├── 📄 dataset_generator3.py
│   │   ├── 📄 diophantine_experiments7.py
│   │   ├── 📄 ecdsa_leakage_model4.py
│   │   ├── 📄 ecdsa_simulator2.py
│   │   ├── 📄 hnp_attack1.py
│   │   ├── 📄 hnp_utils.py
│   │   ├── 📄 main.py
│   │   └── 📄 theory_to_experiment_bridge8.py
│   │
│   ├── 📁 artifacts/
│   │   ├── 📄 bridge_results.json
│   │   ├── 📄 cnn_predictions.json
│   │   ├── 📄 diophantine_results.json
│   │   ├── 📄 hnp_result.json
│   │   ├── 📄 oracle_dataset.json
│   │   ├── 📄 pipeline_results.json
│   │   ├── 📄 dataset_metadata.json
│   │   ├── 📄 dataset_split.json
│   │   ├── 📄 profile_dataset.json
│   │   ├── 📄 public_dataset.json
│   │   ├── 🖼️ diophantine_bridge_verification.png
│   │   └── 📄 README.txt
│   │
│   ├── 📁 cnn/
│   │   ├── 📄 config.yaml
│   │   ├── 📄 evaluate.py
│   │   ├── 📄 export_weights.py
│   │   ├── 📄 model.py
│   │   ├── 📄 train.py
│   │   └── 📄 utils.py
│   │
│   ├── 📁 dataset/
│   │   ├── 📁 sample/
│   │   │   ├── 📄 public_dataset.json
│   │   │   ├── 📄 profile_dataset.json
│   │   │   ├── 📄 dataset_metadata.json
│   │   │   └── 📄 dataset_split.json
│   │   │
│   │   ├── 📄 README.md
│   │   ├── 📄 generate.py
│   │   └── 📄 capture_esp32.py
│   │
│   ├── 📁 esp32/
│   │   ├── 📁 firmware/
│   │   │   ├── 📄 CMakeLists.txt
│   │   │   ├── 📄 sdkconfig.defaults
│   │   │   │
│   │   │   └── 📁 main/
│   │   │       ├── 📄 CMakeLists.txt
│   │   │       ├── 📄 main.c
│   │   │       ├── 📄 ecdsa_exp.c
│   │   │       └── 📄 ecdsa_exp.h
│   │   │
│   │   ├── 📄 README.md
│   │   └── 📄 protocol.md
│   │
│   ├── 📁 experiments/
│   │   ├── 📄 benchmark_cnn_hnp_surface.py
│   │   ├── 📄 benchmark_hnp_thresholds.py
│   │   └── 📄 run_experiments.py
│   │
│   ├── 📁 lattice/
│   │   ├── 📄 README.md
│   │   ├── 📄 solver.py
│   │   └── 📄 __init__.py
│   │
│   └── 📁 simulator/
│       ├── 📄 cli.py
│       ├── 📄 config.py
│       ├── 📄 generator.py
│       └── 📄 noise.py
│
├── 📁 matematica/
│   │
│   └── 📁 experiments/
│       │
│       ├── 📁 conurenta/
│       │   ├── 📁 figs_con/
│       │   │   └── 🖼️ ...
│       │   └── 🖼️ ...
│       │
│       ├── 📁 graficele_matematice/
│       │   ├── 📁 figs_cert/
│       │   │   └── 🖼️ ...
│       │   └── 🖼️ ...
│       │
│       ├── 📁 paper/
│       │   ├── 📄 arithmetic_concurrence.pdf
│       │   └── 📄 arithmetic_concurrence.tex
│       │
│       └── 📄 README.md
│
├── 📁 misc/
│   └── 🖼️ ...
│
├── 📁 notebooks/
│   ├── 📓 01_data-generation.ipynb
│   ├── 📓 02_baseline.ipynb
│   ├── 📓 03_cnn-training.ipynb
│   └── 📓 04_final-results.ipynb
│
├── 📁 results/
│   ├── 📁 figures/
│   │   └── 🖼️ ...
│   │
│   └── 📁 tables/
│       └── 📄 final-results.csv
│
├── 📁 tests/
│   ├── 📄 test_ecdsa.py
│   ├── 📄 test_hnp.py
│   ├── 📄 test_math.py
│   ├── 📄 test_model.py
│   ├── 📄 test_simulator.py
│   └── 📄 test_smoke.py
│
├── 📁 website/
│   ├── 📄 index.html
│   ├── 📄 app.js
│   ├── 📄 styles.css
│   ├── 📄 data.js
│   ├── 📄 README.md
│   ├── 📄 DESIGN-HANDOFF.md
│   │
│   ├── 📁 paper/ (*)
│   │   └── 📄 Recovery_of_private_keys.pdf
│   │
│   └── 📁 source/
│       ├── 📁 informatica/ (*)
│       │   ├── 📄 hnp_attack1.py
│       │   ├── 📄 ecdsa_leakage_model4.py
│       │   ├── 📄 cnn_utils.py
│       │   ├── 📄 cnn_model.py
│       │   ├── 📄 dataset_generator3.py
│       │   ├── 📄 cnn_config.yaml
│       │   └── 📄 Lattice_key6.py
│       │
│       └── 📁 results/ (*)
│           ├── 📄 hnp_result.json
│           ├── 📄 pipeline_results.json
│           ├── 📄 dataset_metadata.json
│           └── 📄 final-results.csv
├── 📄 .gitignore
├── 📄 ARCHITECTURE.md
├── 📄 CITATION.cff
├── 📄 LICENSE
├── 📄 Q&A.txt
├── 📄 README.md
└── 📄 requirements.txt
```

> Structura se va modifica în timp dacă este necesară
(*) Este preluat doar ce este nevoie din sistemul deja existent.
