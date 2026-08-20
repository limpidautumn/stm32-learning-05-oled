#include "oled.hpp"

#include "stm32f1xx_hal_def.h"

Ssd1309::Ssd1309() {}

void Ssd1309::init() {
  u8g2_Setup_ssd1309_i2c_128x64_noname2_f(&u8g2_, U8G2_R0, i2cByteCb,
                                          gpioDelayCb);
  u8g2_SetI2CAddress(&u8g2_, 0x7A);
  u8g2_InitDisplay(&u8g2_);
  u8g2_SetPowerSave(&u8g2_, 0);
}

void Ssd1309::print(const char *str) {
  u8g2_ClearBuffer(&u8g2_);
  u8g2_SetFont(&u8g2_, u8g2_font_ncenR12_tr);
  u8g2_DrawStr(&u8g2_, 0, 12, str);
  u8g2_SendBuffer(&u8g2_);
}

uint8_t Ssd1309::i2cByteCb(u8x8_t *u8x8, uint8_t msg, uint8_t argInt,
                           void *argPtr) {
  uint8_t *data = static_cast<uint8_t *>(argPtr);

  static uint8_t buf[64];
  static uint8_t idx = 0;

  switch (msg) {
  case U8X8_MSG_BYTE_SEND:

    while (argInt-- > 0)
      buf[idx++] = *data++;
    break;

  case U8X8_MSG_BYTE_INIT:
    break;
  case U8X8_MSG_BYTE_SET_DC:
    break;
  case U8X8_MSG_BYTE_START_TRANSFER:
    idx = 0;
    break;
  case U8X8_MSG_BYTE_END_TRANSFER:
    HAL_I2C_Master_Transmit(&hi2c1, u8x8_GetI2CAddress(u8x8), buf, idx, 1000);
    break;
  default:
    return 0;
  }
  return 1;
}

uint8_t Ssd1309::gpioDelayCb(u8x8_t *u8x8, uint8_t msg, uint8_t argInt,
                             void *argPtr) {
  UNUSED(argPtr);
  switch (msg) {
  case U8X8_MSG_DELAY_MILLI:
    HAL_Delay(argInt);
    break;
  default:
    u8x8_SetGPIOResult(u8x8, 1);
    break;
  }
  return 1;
}
