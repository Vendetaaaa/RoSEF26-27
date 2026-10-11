from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROTO = (ROOT / "src/protocol/protocol.cpp").read_text(encoding="utf-8")
BATCH = (ROOT / "src/batch/batch.cpp").read_text(encoding="utf-8")
BATCH_H = (ROOT / "src/batch/batch.h").read_text(encoding="utf-8")
README = (ROOT / "README.md").read_text(encoding="utf-8")


def test_sbatch_is_advertised_as_gpio_only_pulse_train():
    assert 'SBATCH [count]' in PROTO
    assert 'GPIO4-only pulse train' in PROTO
    assert '`SBATCH [count]`' in README


def test_sbatch_handler_toggles_gpio4_directly():
    handler = PROTO.split('void handleSbatch(', 1)[1].split('void handleFinal()', 1)[0]
    assert 'pinMode(TRIGGER_GPIO, OUTPUT)' in handler
    assert 'digitalWrite(TRIGGER_GPIO, TRIGGER_ACTIVE_LEVEL)' in handler
    assert 'digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL)' in handler
    assert 'delay(SBATCH_LOW_MS)' in handler
    assert 'delay(SBATCH_HIGH_MS)' in handler
    assert 'DEFAULT_SBATCH_COUNT = 10' in handler


def test_sbatch_does_not_depend_on_crypto_or_batch_runner():
    handler = PROTO.split('void handleSbatch(', 1)[1].split('void handleFinal()', 1)[0]
    assert 'sbatchRun(' not in handler
    assert 'experimentSignDigest(' not in handler
    assert 'batchRun(' not in handler
    assert 'MODE=GPIO_ONLY' in handler
    assert 'bool sbatchRun(' not in BATCH
    assert 'sbatchRun(' not in BATCH_H


def test_sbatch_accepts_no_argument_space_and_tab_forms():
    assert "command[6] == '\\0'" in PROTO
    assert "command[6] == ' '" in PROTO
    assert "command[6] == '\\t'" in PROTO
    assert 'handleSbatch(command + 6);' in PROTO


def test_sbatch_returns_gpio_readback_status_and_idle_high():
    handler = PROTO.split('void handleSbatch(', 1)[1].split('void handleFinal()', 1)[0]
    assert 'digitalWrite(TRIGGER_GPIO, TRIGGER_IDLE_LEVEL);' in handler
    assert 'digitalRead(TRIGGER_GPIO) == TRIGGER_ACTIVE_LEVEL' in handler
    assert 'digitalRead(TRIGGER_GPIO) == TRIGGER_IDLE_LEVEL' in handler
    assert 'OK SBATCH=PASS' in handler
    assert 'ERR SBATCH_GPIO_READBACK' in handler
