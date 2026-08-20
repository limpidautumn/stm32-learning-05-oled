#pragma once

#include "i2c.h"
#include "u8g2.h"

class Ssd1309 {
public:
  Ssd1309();
  void init();
  void print(const char *str);

private:
  u8g2_t u8g2_;

  static uint8_t i2cByteCb(u8x8_t *u8x8, uint8_t msg, uint8_t argInt,
                           void *argPtr);
  static uint8_t gpioDelayCb(u8x8_t *u8x8, uint8_t msg, uint8_t argInt,
                             void *argPtr);
};
