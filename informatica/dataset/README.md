# Dataset

`generate.py` creates the reproducible ECDSA public, oracle, and CNN profile datasets.

```bash
python informatica/dataset/generate.py --samples 160 --leaked-bits 12 --noise 0.15
```

The generated oracle contains the private key and complete nonces and should stay out of the public dataset. `capture_esp32.py` records ESP32 signature metadata; physical traces are captured separately by the measurement setup.


Datasetul implicit folosește 160 de semnături, 12 biți MSB, `sigma = 0.15` și split-ul 96 train / 24 validation / 40 test. Fișierul `informatica/artifacts/dataset_split.json` este sursa folosită de CNN și HNP.
