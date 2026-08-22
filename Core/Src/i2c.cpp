#include "i2c.hpp"

namespace i2c {

void loop() { Bus::loop(); }

Bus i2c1(hi2c1);

} // namespace i2c

extern "C" void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  i2c::Bus::dispatch(hi2c, 1);
}
extern "C" void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  i2c::Bus::dispatch(hi2c, 1);
}
extern "C" void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  i2c::Bus::dispatch(hi2c, 1);
}
extern "C" void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  i2c::Bus::dispatch(hi2c, 1);
}
extern "C" void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
  i2c::Bus::dispatch(hi2c, 0);
}
