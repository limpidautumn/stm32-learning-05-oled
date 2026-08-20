#include "oled.hpp"

namespace oled {

Ssd1309 oled;

void init() {
  oled.init();
  oled.print("Test");
}

} // namespace oled
