#include "led.hpp"

#include "main.h"

namespace led {

void loop() {
  static StatusLed statusLed(LED_R_GPIO_Port, LED_R_Pin);
  statusLed.advanceState();
}

} // namespace led
