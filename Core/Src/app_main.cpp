#include "app_main.hpp"
#include "led.hpp"
#include "oled.hpp"

void cpp_setup() { oled::init(); }

void cpp_loop() {
  while (true) {
    led::loop();
  }
}
