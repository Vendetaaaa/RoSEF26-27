#include "button.h"

#include "../core/config.h"

namespace rosef {

namespace {

constexpr uint32_t DEBOUNCE_MS = 30;

bool stable_state = HIGH;
bool last_raw_state = HIGH;
uint32_t last_change_ms = 0;
bool pressed_event = false;

}

void buttonBegin() {
  pinMode(BUTTON_GPIO, INPUT_PULLUP);

  const bool initial_state =
      digitalRead(BUTTON_GPIO);

  stable_state = initial_state;
  last_raw_state = initial_state;
  last_change_ms = millis();
  pressed_event = false;
}

bool buttonIsPressed() {
  const bool raw_state =
      digitalRead(BUTTON_GPIO);

  if (raw_state != last_raw_state) {
    last_raw_state = raw_state;
    last_change_ms = millis();
  }

  if ((millis() - last_change_ms) >= DEBOUNCE_MS &&
      raw_state != stable_state) {

    const bool previous_state = stable_state;
    stable_state = raw_state;

    if (previous_state == HIGH &&
        stable_state == LOW) {
      pressed_event = true;
    }
  }

  return stable_state == LOW;
}

bool buttonWasPressed() {
  buttonIsPressed();

  if (!pressed_event) {
    return false;
  }

  pressed_event = false;
  return true;
}

}
