#pragma once

#include "main.h"
#include "stm32f1xx_hal.h"

namespace led {

void loop();

class StatusLed {
public:
  StatusLed(GPIO_TypeDef *const ledPort, const uint16_t ledPin,
            const uint32_t flashingInterval = 1000)
      : ledPort(ledPort), ledPin(ledPin), flashingInterval(flashingInterval) {}

  void advanceState() {
    switch (state.get()) {
    case StateEnum::Idle:
      if (state.timeElapsed() < flashingInterval)
        break;
      state.enter(StateEnum::Switching);
      break;
    case StateEnum::Switching:
      HAL_GPIO_TogglePin(ledPort, ledPin);
      state.enter(StateEnum::Idle);
      break;
    }
  }

private:
  GPIO_TypeDef *const ledPort;
  const uint16_t ledPin;
  const uint32_t flashingInterval;

  enum class StateEnum : uint8_t {
    Idle,
    Switching,
  };

  class State {
  public:
    State(StateEnum s) { enter(s); }
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

} // namespace led
