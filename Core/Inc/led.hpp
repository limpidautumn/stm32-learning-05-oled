#pragma once

#include "utils.hpp"

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

  utils::State<StateEnum> state{StateEnum::Idle};
};

} // namespace led
