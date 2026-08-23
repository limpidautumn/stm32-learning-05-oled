#include "app_main.hpp"
#include "aht20.hpp"
#include "i2c.hpp"
#include "key.hpp"
#include "led.hpp"
#include "oled.hpp"

void cpp_setup() {
  aht20::setup();
  oled::setup();
}

void cpp_loop() {
  while (true) {
    i2c::loop();
    key::loop();
    aht20::loop();
    oled::loop();
    led::loop();
  }
}
