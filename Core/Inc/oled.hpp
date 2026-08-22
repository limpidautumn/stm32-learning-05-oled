#pragma once

#include "i2c.hpp"

#include "cstring"
#include "main.h"
#include "stm32f1xx_hal_i2c.h"
#include "u8g2.h"
#include <cstddef>

namespace oled {

void init();
void loop();

class Ssd1309 {

private:
  static constexpr uint8_t pageHeight = 8; // 1 Page = 8 Rows
  static constexpr uint8_t columnCnt = 128;
  static constexpr uint8_t pageCnt = 8;
  static constexpr uint8_t rowCnt = pageHeight * pageCnt;
  static constexpr uint16_t byteCnt = columnCnt * pageCnt;

public:
  Ssd1309(i2c::Bus &i2c, const uint16_t &addr) : i2c(i2c), devAddr(addr) {}

  void init() {
    u8g2_SetupDisplay(&u8g2_, u8x8_d_ssd1309_128x64_noname2,
                      u8x8_cad_ssd13xx_fast_i2c, i2cByteCb, gpioDelayCb);
    u8g2_SetupBuffer(&u8g2_, frameBuf_, pageHeight,
                     u8g2_ll_hvline_vertical_top_lsb, U8G2_R0);
    u8g2_SetUserPtr(&u8g2_, this);
    u8g2_InitDisplay(&u8g2_);
    u8g2_SetPowerSave(&u8g2_, 0);
  }

  void drawRev() {
    static uint16_t revStPos = 0;
    for (uint16_t i = revStPos; i < revStPos + (byteCnt >> 1); ++i)
      frameBuf_[i % byteCnt] ^= 0xFFu;
    revStPos = (revStPos + 1) % byteCnt;
  }

  u8g2_t *u8g2() { return &u8g2_; }

  static constexpr const uint8_t &kWidth = columnCnt;
  static constexpr const uint8_t &kHeight = rowCnt;

private:
  i2c::Bus &i2c;
  const uint16_t devAddr;
  u8g2_t u8g2_;

  // 8 vertical pixels per byte, LSB on top
  uint8_t frameBuf_[byteCnt];

  static uint8_t i2cByteCb(u8x8_t *u8x8, uint8_t msg, uint8_t argInt,
                           void *argPtr) {
    auto *self = static_cast<Ssd1309 *>(u8g2_GetUserPtr(u8x8));
    if (self == nullptr)
      return 0;

    uint8_t *data = static_cast<uint8_t *>(argPtr);

    static uint8_t buf[64];
    static uint8_t idx = 0;

    switch (msg) {
    case U8X8_MSG_BYTE_SEND:
      while (argInt-- > 0)
        buf[idx++] = *data++;
      return 1;
    case U8X8_MSG_BYTE_INIT:
      return 1;
    case U8X8_MSG_BYTE_SET_DC:
      return 1;
    case U8X8_MSG_BYTE_START_TRANSFER:
      idx = 0;
      return 1;
    case U8X8_MSG_BYTE_END_TRANSFER:
      self->i2c.write(self->devAddr, buf, idx);
      return 1;
    default:
      return 0;
    }
  }

  static uint8_t gpioDelayCb(u8x8_t *u8x8, uint8_t msg, uint8_t argInt,
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
};

class Screen {
public:
  using RefreshHandeler = void (*const)();

  Screen(const uint32_t refreshInterval, RefreshHandeler onRefresh)
      : refreshInterval(refreshInterval), onRefresh(onRefresh) {}

  void advanceState() {
    switch (state.get()) {
    case StateEnum::Idle:
      if (state.timeElapsed() < refreshInterval)
        break;
      state.enter(StateEnum::Refreshing);
      break;
    case StateEnum::Refreshing:
      onRefresh();
      state.enter(StateEnum::Idle);
      break;
    }
  }

private:
  const uint32_t refreshInterval;
  RefreshHandeler onRefresh;

  enum class StateEnum : uint8_t {
    Idle,
    Refreshing,
  };

  class State {
  public:
    State(const StateEnum &s) { enter(s); }
    StateEnum get() { return state; }
    void enter(const StateEnum &s) {
      state = s;
      entryTime = HAL_GetTick();
    }
    uint32_t timeElapsed() { return HAL_GetTick() - entryTime; }

  private:
    StateEnum state;
    uint32_t entryTime;
  };
  State state{StateEnum::Idle};
};

} // namespace oled
