from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROTO = (ROOT / "src/protocol/protocol.cpp").read_text(encoding="utf-8")
RGB = (ROOT / "src/rgbled/rgbled.cpp").read_text(encoding="utf-8")
RGB_H = (ROOT / "src/rgbled/rgbled.h").read_text(encoding="utf-8")
CONFIG = (ROOT / "src/core/config.h").read_text(encoding="utf-8")
OLED = (ROOT / "src/oled/oled.cpp").read_text(encoding="utf-8")
OLED_H = (ROOT / "src/oled/oled.h").read_text(encoding="utf-8")
OLED_LOGO = (ROOT / "src/oled/leakon_logo.h").read_text(encoding="utf-8")


def test_every_dispatched_command_is_wrapped_for_status_led_and_oled():
    assert "void dispatchLine(\n    char *command)" in PROTO
    assert "void handleLine(char *command)" in PROTO
    assert "rgbLedSetCommandRunning();" in PROTO
    assert "oledShowCommandRunning(command);" in PROTO
    assert "finishCommandRgbStatus();" in PROTO
    assert "oledShowCommandResult(!g_command_failed);" in PROTO


def test_led_is_one_wire_neopixel_on_gpio25_with_requested_colors():
    assert "constexpr uint8_t RGB_LED_GPIO = 25;" in CONFIG
    assert "Adafruit_NeoPixel pixel(" in RGB
    assert "RGB_LED_GPIO," in RGB
    assert "void rgbLedSetIdle() { setColor(0, 255, 0); }" in RGB
    assert "void rgbLedSetCommandRunning() { setColor(255, 72, 0); }" in RGB
    assert "void rgbLedSetFail() { setColor(255, 0, 0); }" in RGB
    assert "void rgbLedSetPass() { rgbLedSetIdle(); }" in RGB


def test_oled_uses_second_i2c_bus_and_autodetects_address():
    assert "constexpr uint8_t OLED_SDA_GPIO = 18;" in CONFIG
    assert "constexpr uint8_t OLED_SCL_GPIO = 19;" in CONFIG
    assert "TwoWire oledWire(1);" in OLED
    assert "OLED_I2C_ADDRESS_PRIMARY" in OLED
    assert "OLED_I2C_ADDRESS_ALTERNATE" in OLED
    assert "bool oledBegin();" in OLED_H
    assert 'strcmp(command, "OLEDTEST") == 0' in PROTO


def test_diagnostic_pin_test_does_not_toggle_oled_i2c_pins():
    pins_block = PROTO.split("constexpr uint8_t TEST_PINS[] = {", 1)[1].split("};", 1)[0]
    assert "18" not in pins_block
    assert "19" not in pins_block


def test_command_errors_mark_status_led_red_and_unknown_commands_still_fail():
    assert 'commandFailureLine(F("ERR UNKNOWN_COMMAND"));' in PROTO
    assert 'commandFailureLine(F("ERR INA219_UNAVAILABLE"));' in PROTO
    assert "rgbLedSetFail();" in PROTO


def test_oled_retries_initialization_from_oledtest_command():
    block = PROTO.split('if (strcmp(command, "OLEDTEST") == 0) {', 1)[1].split('return;', 1)[0]
    assert '!oledAvailable() && !oledBegin()' in block
    assert 'ERR OLED_UNAVAILABLE' in block


def test_oled_initialization_logs_each_stage_and_uses_controller_one():
    assert 'TwoWire oledWire(1);' in OLED
    assert 'OLED_I2C_BEGIN=OK' in OLED
    assert 'OLED_PROBE_0x' in OLED
    assert 'OLED_SSD1306_INIT=OK' in OLED
    assert 'OLED_READY=1' in OLED


def test_oled_is_initialized_before_ina219_setup():
    begin = PROTO.split('void protocolBegin() {', 1)[1].split('void protocolLoop()', 1)[0]
    assert begin.index('oledBegin()') < begin.index('experimentBegin()')


def test_oled_design_uses_graphical_panels_without_led_color_text():
    assert "drawRoundRect" in OLED
    assert "drawMiniMark" in OLED
    assert "drawActivityMeter" in OLED
    assert "LEAKON_LOGO_BITMAP" in OLED
    assert "drawBootAnimation" in OLED
    assert "SIGNAL > LEARNING > RECOVERY" in OLED
    assert "LED: ORANGE" not in OLED
    assert "LED: GREEN" not in OLED
    assert "LED: RED" not in OLED
    assert 'F("LEAKON")' in OLED
    assert 'LEAKAGE & NEURONAL ANALYSIS' in OLED
    assert 'ECDSA' in OLED and 'CNN1D' in OLED and 'HNP+LLL' in OLED


def test_leakon_wordmark_asset_and_microtype_are_ready_for_128x64():
    assert "LEAKON_LOGO_WIDTH = 124" in OLED_LOGO
    assert "LEAKON_LOGO_HEIGHT = 15" in OLED_LOGO
    assert "LEAKON_LOGO_BITMAP" in OLED_LOGO
    assert "drawTinyCentered" in OLED
    assert "fillRoundRect(44, 13, 16, 15, 2, SSD1306_WHITE)" in OLED
    assert "LEAKAGE & NEURONAL ANALYSIS" in OLED
    assert "SIGNAL > LEARNING > RECOVERY" in OLED


def test_modified_oled_code_has_no_comments_or_docstrings():
    for source in (OLED, OLED_H, OLED_LOGO):
        assert "//" not in source
        assert "/*" not in source
        assert "*/" not in source
