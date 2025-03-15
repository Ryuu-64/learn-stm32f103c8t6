#ifndef HARDWARE_SSD1306_H
#define HARDWARE_SSD1306_H

#include "stm32f1xx_hal.h"

// void Hardware_SSD1306_Init(I2C_HandleTypeDef *hi2c_input, uint8_t width_input, uint8_t height_input);

void Hardware_SSD1306_Display(const uint8_t *chars);

#endif //HARDWARE_SSD1306_H
