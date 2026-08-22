#include "app_main.hpp"
#include "i2c.hpp"
#include "led.hpp"
#include "oled.hpp"

#include "main.h"

void cpp_setup() { oled::init(); }

void cpp_loop() {
  while (true) {
    i2c::loop();
    oled::loop();
    led::loop();
  }
}
