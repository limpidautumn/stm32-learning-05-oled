#include "app_main.hpp"
#include "oled.hpp"

#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"

Ssd1309 oled;

void cpp_setup() {
  oled.init();
  oled.print("Hello, world!");
}

void cpp_loop() {
  while (true) {
    HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
    HAL_Delay(1000);
  }
}
