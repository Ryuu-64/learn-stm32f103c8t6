#ifndef HARDWARE_SSD1306_TEST_H
#define HARDWARE_SSD1306_TEST_H

#include "stm32f1xx_hal_i2c.h"

void Hardware_SSD1306_Init_Test(I2C_HandleTypeDef *hi2c_input, uint8_t width_input, uint8_t height_input);

#endif //HARDWARE_SSD1306_TEST_H
