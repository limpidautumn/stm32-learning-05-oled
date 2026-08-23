#include "oled.hpp"
#include "aht20.hpp"
#include "i2c.hpp"
#include "key.hpp"
#include "utils.hpp"

#include "u8g2.h"
#include "u8g2_font_unifont_hello.h"
#include <cstdio>

namespace oled {

class TestText {
public:
  void getText(char *str, size_t size) {
    switch (state.get()) {
    case StateEnum::Chinese:
      snprintf(str, size, "你好，世界。");
      break;
    case StateEnum::English:
      snprintf(str, size, "Hello, world.");
      break;
    case StateEnum::Japanese:
      snprintf(str, size, "こんにちは世界。");
      break;
    case StateEnum::Russian:
      snprintf(str, size, "Привет, мир.");
      break;
    }
  }
  enum class StateEnum : uint8_t {
    Chinese,
    English,
    Japanese,
    Russian,
  };
  void advanceState() {
    switch (state.get()) {
    case StateEnum::Chinese:
      state.enter(StateEnum::English);
      break;
    case StateEnum::English:
      state.enter(StateEnum::Japanese);
      break;
    case StateEnum::Japanese:
      state.enter(StateEnum::Russian);
      break;
    case StateEnum::Russian:
      state.enter(StateEnum::Chinese);
      break;
    }
  }

private:
  utils::State<StateEnum> state{StateEnum::Chinese};
} testText;

class Aht20Temp {
public:
  void getText(char *str, size_t size) {
    switch (state.get()) {
    case StateEnum::Celsius:
      snprintf(str, size, "Temp: +%.2d.%.2dC", static_cast<int>(aht20::tp),
               static_cast<int>(100 * aht20::tp) % 100);
      break;
    case StateEnum::Fahrenheit:
      const double f = 1.8 * aht20::tp + 32.0;
      snprintf(str, size, "Temp: +%.3d.%.2dF", static_cast<int>(f),
               static_cast<int>(100 * f) % 100);
      break;
    }
  }
  enum class StateEnum : uint8_t {
    Celsius,
    Fahrenheit,
  };
  void advanceState() {
    switch (state.get()) {
    case StateEnum::Celsius:
      state.enter(StateEnum::Fahrenheit);
      break;
    case StateEnum::Fahrenheit:
      state.enter(StateEnum::Celsius);
      break;
    }
  }

private:
  utils::State<StateEnum> state{StateEnum::Celsius};
} aht20Temp;

void uint32_to_hex(uint32_t value, char *out) {
  static constexpr char digits[] = "0123456789ABCDEF";

  for (int i = 7; i >= 0; --i) {
    out[i] = digits[value & 0x0F];
    value >>= 4;
  }
  out[8] = '\0';
}

Ssd1309 oled(i2c::i2c1, 0x7A);

void setup() {
  oled.init();
  key::key2.setHook([](bool pressed, void *) {
    if (pressed)
      testText.advanceState();
  });
  key::key1.setHook([](bool pressed, void *) {
    if (pressed)
      aht20Temp.advanceState();
  });
}

void refresh() {
  static char str[255];
  u8g2_ClearBuffer(oled.u8g2());

  // Test text
  testText.getText(str, sizeof(str));
  u8g2_SetFont(oled.u8g2(), u8g2_font_unifont_hello);
  u8g2_DrawUTF8(oled.u8g2(), 0, 16, str);

  // Temp
  aht20Temp.getText(str, sizeof(str));
  u8g2_SetFont(oled.u8g2(), u8g2_font_t0_12_tf);
  u8g2_DrawUTF8(oled.u8g2(), 0, 28, str);

  // RH
  snprintf(str, sizeof(str), "RH: %.2d.%.1d%%", static_cast<int>(aht20::rh),
           static_cast<int>(10 * aht20::rh) % 10);
  u8g2_SetFont(oled.u8g2(), u8g2_font_t0_12_tf);
  u8g2_DrawUTF8(oled.u8g2(), 0, 40, str);

  // HAL_GetTick
  char hexTick[9] = "";
  uint32_to_hex(HAL_GetTick(), hexTick);
  snprintf(str, sizeof(str), "Tick: 0x%s\n", hexTick);
  u8g2_SetFont(oled.u8g2(), u8g2_font_t0_12_tf);
  u8g2_DrawUTF8(oled.u8g2(), 0, 52, str);

  oled.drawRev();
  u8g2_SendBuffer(oled.u8g2());
}

Screen screen(40, refresh);

void loop() { screen.advanceState(); }

} // namespace oled
