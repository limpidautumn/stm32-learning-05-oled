#include "led.hpp"

namespace led {

void loop() {
  static StatusLed statusLed;
  statusLed.advanceState();
}

} // namespace led
