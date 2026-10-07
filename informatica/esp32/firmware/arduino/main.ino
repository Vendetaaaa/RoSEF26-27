#include "src/protocol/protocol.h"

void setup() {
  rosef::protocolBegin();
}

void loop() {
  rosef::protocolLoop();
}