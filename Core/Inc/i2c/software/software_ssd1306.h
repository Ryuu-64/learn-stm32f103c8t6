#ifndef SOFTWARE_SSD1306_H
#define SOFTWARE_SSD1306_H

#include "stm32f1xx_hal.h"

void Software_SSD1306_Init(uint8_t width_input, uint8_t height_input);

void Software_SSD1306_Display(const uint8_t *chars);

#endif //SOFTWARE_SSD1306_H
