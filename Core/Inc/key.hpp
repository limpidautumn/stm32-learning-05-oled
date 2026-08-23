#pragma once

#include "utils.hpp"

namespace key {

void loop();

class Key : public utils::InstanceRegistry<Key> {
public:
  // Edge callback. pressed: true (falling edge) / false (rising edge).
  using Hook = void (*)(bool pressed, void *ctx);

  Key(GPIO_TypeDef *const keyPort, const uint16_t keyPin,
      const uint32_t debounceDuration = 10)
      : keyPort(keyPort), keyPin(keyPin), debounceDuration(debounceDuration) {}

  void setHook(Hook hook, void *ctx = nullptr) {
    hook_ = hook;
    hookCtx_ = ctx;
  }

  void advanceState() {
    GPIO_PinState pinState = HAL_GPIO_ReadPin(keyPort, keyPin);
    switch (state.get()) {
    case StateEnum::High:
      if (pinState == GPIO_PIN_RESET)
        state.enter(StateEnum::FallingEdgeDebouncing);
      break;
    case StateEnum::FallingEdgeDebouncing:
      if (pinState == GPIO_PIN_SET)
        state.enter(StateEnum::High);
      else if (state.timeElapsed() < debounceDuration)
        fire(true), state.enter(StateEnum::Low);
      // Hook A
      break;
    case StateEnum::Low:
      if (pinState == GPIO_PIN_SET)
        state.enter(StateEnum::RisingEdgeDebouncing);
      break;
    case StateEnum::RisingEdgeDebouncing:
      if (pinState == GPIO_PIN_RESET)
        state.enter(StateEnum::Low);
      else if (state.timeElapsed() < debounceDuration)
        fire(false), state.enter(StateEnum::High);
      // Hook B
      break;
    }
  }

  static void loop() {
    forEach([](Key *k) { k->advanceState(); });
  }

private:
  GPIO_TypeDef *const keyPort;
  const uint16_t keyPin;
  const uint32_t debounceDuration;

  enum class StateEnum : uint8_t {
    High,
    FallingEdgeDebouncing,
    Low,
    RisingEdgeDebouncing,
  };

  utils::State<StateEnum> state{StateEnum::High};

  Hook hook_ = nullptr;
  void *hookCtx_ = nullptr;
  void fire(bool pressed) {
    if (hook_)
      hook_(pressed, hookCtx_);
  }
};

extern Key key1, key2;

} // namespace key
