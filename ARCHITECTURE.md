# Arhitectura proiectului:

```
📁 RoSEF26-27/
├── 📁 .github/
│   ├── 📁 workflows/
│   │   ├── 📄 reproducibility.yml
│   │   └── 📄 tests.yml
│   └── 📄 pull_request_template.md
│
├── 📁 docs/
│   ├── 📄 ethics.md
│   ├── 📄 experimental-protocol.md
│   ├── 📄 methodology.md
│   ├── 📄 project-overview.md
│   ├── 📄 reproducibility.md
│   └── 📄 research-question.md
│
├── 📁 informatica/
│   ├── 📁 ECDSA/
│   │   ├── 📄 Lattice_key6.py
│   │   ├── 📄 cnn_nonce_analysis5.py
│   │   ├── 📄 dataset_generator3.py
│   │   ├── 📄 diophantine_experiments7.py
│   │   ├── 📄 ecdsa_leakage_model4.py
│   │   ├── 📄 ecdsa_simulator2.py
│   │   ├── 📄 hnp_attack1.py
│   │   ├── 📄 main.py
│   │   └── 📄 theory_to_experiment_bridge8.py
│   │
│   ├── 📁 artifacts/
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
│   │   ├── 📄 README.md
│   │   ├── 📄 generate.py
│   │   └── 📄 capture_esp32.py
│   │
│   ├── 📁 esp32/
│   │   ├── 📁 firmware/
│   │   │   ├── 📄 CMakeLists.txt
│   │   │   ├── 📄 sdkconfig.defaults
│   │   │   └── 📁 main/
│   │   │       ├── 📄 CMakeLists.txt
│   │   │       ├── 📄 main.c
│   │   │       ├── 📄 ecdsa_exp.c
│   │   │       └── 📄 ecdsa_exp.h
│   │   ├── 📄 README.md
│   │   └── 📄 protocol.md
│   │
│   ├── 📁 experiments/
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
│   └── 📁 experiments/
│       ├── 📁 conurenta/
│       │   ├── 📁 figs_con/
│       │   │   └── 🖼️ . . .
│       │   └── 🖼️ . . .
│       ├── 📁 graficele_matematice/
│       │   ├── 📁 figs_cert/
│       │   │   └── 🖼️ . . .
│       │   └── 🖼️ . . .
│       ├── 📁 paper/
│       │   ├── 📄 arithmetic_concurrence.pdf
│       │   └── 📄 arithmetic_concurrence.tex
│       └── 📄 README.md
│
├── 📁 misc/
│   └── 🖼️ . . .
│
├── 📁 notebooks/
│   ├── 📄 01_data-generation.ipynb
│   ├── 📄 02_baseline.ipynb
│   ├── 📄 03_cnn-training.ipynb
│   └── 📄 04_final-results.ipynb
│
├── 📁 results/
│   ├── 📁 figures/
│   │   └── 🖼️ . . .
│   └── 📁 tables/
│       └── 📄 final-results.csv
│
├── 📁 tests/
│   ├── 📄 test_math.py
│   ├── 📄 test_model.py
│   ├── 📄 test_simulator.py
│   ├── 📄 test_ecdsa.py
│   ├── 📄 test_hnp.py
│   └── 📄 test_smoke.py
│
├── 📄 .gitignore
├── 📄 ARCHITECTURE.md
├── 📄 CITATION.cff
├── 📄 LICENSE (MIT)
├── 📄 README.md
├── 📄 Q&A.txt
└── 📄 requirements.txt
```

> Structura se va modifica în timp dacă este necesară
