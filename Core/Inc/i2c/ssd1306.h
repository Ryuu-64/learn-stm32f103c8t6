//
// Created by Ryuu on 2025/3/8.
//

#ifndef SSD1306_H
#define SSD1306_H

#include "stm32f1xx_hal.h"

#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64
#define SSD1306_BUFFER_SIZE     (SSD1306_WIDTH * SSD1306_HEIGHT / 8)
#define SSD1306_PAGE_COUNT      (SSD1306_HEIGHT / 8)
#define SSD1306_I2C_ADDR        0x3C << 1
#define SSD1306_CMD_CTRL_BYTE   0x00
#define SSD1306_DATA_CTRL_BYTE  0x40

void SSD1306_Init(I2C_HandleTypeDef *hi2c_input, uint8_t width_input, uint8_t height_input);

void SSD1306_Display(const uint8_t *chars);

#endif //SSD1306_H
