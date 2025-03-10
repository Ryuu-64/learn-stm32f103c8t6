#ifndef SSD1306_H
#define SSD1306_H

#include "stm32f1xx_hal.h"

void SSD1306_Init(I2C_HandleTypeDef *hi2c_input, uint8_t width_input, uint8_t height_input);

void SSD1306_Display(const uint8_t *chars);

#endif //SSD1306_H
