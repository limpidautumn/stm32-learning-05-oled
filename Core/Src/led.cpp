#include "led.hpp"
#include "main.h"

namespace led {

void loop() {
  static StatusLed statusLed(LED_B_GPIO_Port, LED_B_Pin);
  statusLed.advanceState();
}

} // namespace led
