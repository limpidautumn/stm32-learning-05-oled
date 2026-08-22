#include "oled.hpp"

#include "i2c.h"
#include "i2c.hpp"
#include "stm32f1xx_hal.h"
#include "u8g2.h"

namespace oled {

void uint32_to_hex(uint32_t value, char *out) {
  static constexpr char digits[] = "0123456789ABCDEF";

  for (int i = 7; i >= 0; --i) {
    out[i] = digits[value & 0x0F];
    value >>= 4;
  }
  out[8] = '\0';
}

Ssd1309 oled(i2c::i2c1, 0x7A);

void init() { oled.init(); }

void refresh() {
  u8g2_ClearBuffer(oled.u8g2());

  // Temp & RH

  // HAL_GetTick
  u8g2_SetFont(oled.u8g2(), u8g2_font_t0_12_tf);
  char str[9];
  uint32_to_hex(HAL_GetTick(), str);
  u8g2_DrawUTF8(oled.u8g2(), 0, 36, str);

  oled.drawRev();
  u8g2_SendBuffer(oled.u8g2());
}

Screen screen(40, refresh);

void loop() { screen.advanceState(); }

} // namespace oled
