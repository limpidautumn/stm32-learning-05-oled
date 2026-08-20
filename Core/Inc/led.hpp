#pragma once

#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"

namespace led {

void loop();

class StatusLed {
private:
  GPIO_TypeDef *const ledPort = LED_R_GPIO_Port;
  const uint16_t ledPin = LED_R_Pin;
  const uint32_t flashingInterval = 1000;

  enum class StateEnum : uint8_t {
    Idle,
    Switching,
  };

  class State {
  private:
    StateEnum state;
    uint32_t entryTime;

  public:
    State(const StateEnum &s) { enter(s); }
    StateEnum get() { return state; }
    void enter(const StateEnum &s) {
      state = s;
      entryTime = HAL_GetTick();
    }
    uint32_t timeElapsed() { return HAL_GetTick() - entryTime; }
  };

public:
  StatusLed() {}

  void advanceState() {
    static State cur(StateEnum::Idle);
    switch (cur.get()) {
    case StateEnum::Idle:
      if (cur.timeElapsed() < flashingInterval)
        break;
      cur.enter(StateEnum::Switching);
      break;
    case StateEnum::Switching:
      HAL_GPIO_TogglePin(ledPort, ledPin);
      cur.enter(StateEnum::Idle);
      break;
    }
  }
};

} // namespace led
