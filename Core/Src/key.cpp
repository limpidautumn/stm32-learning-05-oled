#include "key.hpp"

#include "main.h"

namespace key {

Key key1(KEY_1_GPIO_Port, KEY_1_Pin);
Key key2(KEY_2_GPIO_Port, KEY_2_Pin);

void loop() { Key::loop(); }

} // namespace key
