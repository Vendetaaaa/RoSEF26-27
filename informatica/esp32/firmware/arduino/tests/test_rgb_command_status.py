from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROTO = (ROOT / "src/protocol/protocol.cpp").read_text(encoding="utf-8")
RGB = (ROOT / "src/rgbled/rgbled.cpp").read_text(encoding="utf-8")
RGB_H = (ROOT / "src/rgbled/rgbled.h").read_text(encoding="utf-8")

def test_every_dispatched_command_is_wrapped_for_status_led():
    assert "void dispatchLine(\n    char *command)" in PROTO
    assert "void handleLine(char *command)" in PROTO
    assert "rgbLedSetCommandRunning();" in PROTO
    assert "finishCommandRgbStatus();" in PROTO

def test_led_color_states_are_defined():
    assert "void rgbLedSetCommandRunning();" in RGB_H
    assert "void rgbLedSetCommandRunning() { setColor(true, true, false); }" in RGB
    assert "void rgbLedSetIdle() { setColor(false, false, true); }" in RGB
    assert "void rgbLedSetPass() { setColor(false, true, false); }" in RGB
    assert "void rgbLedSetFail() { setColor(true, false, false); }" in RGB
    assert "void rgbLedSetRunning() { setColor(true, false, true); }" in RGB

def test_idle_blue_requires_live_ina219_and_gpio4_idle():
    assert "ina219Read().valid" in PROTO
    assert "digitalRead(TRIGGER_GPIO) == TRIGGER_IDLE_LEVEL" in PROTO
    assert "g_rgb_hardware_ready = ina_ok || trigger_ok;" in PROTO

def test_failed_command_prints_remain_the_same_but_mark_led_failure():
    assert 'commandFailureLine(F("ERR UNKNOWN_COMMAND"));' in PROTO
    assert 'commandFailureLine(F("ERR INA219_UNAVAILABLE"));' in PROTO
