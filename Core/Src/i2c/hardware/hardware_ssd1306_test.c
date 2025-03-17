#include "i2c/hardware/hardware_ssd1306.h"

#include "stm32f1xx_hal_i2c.h"

void Hardware_SSD1306_Init_Test(I2C_HandleTypeDef *hi2c_input, const uint8_t width_input, const uint8_t height_input) {
    Hardware_SSD1306_Init(hi2c_input, width_input, height_input);
    HAL_Delay(2000);
    Hardware_SSD1306_Display("hello world.");
    HAL_Delay(2000);
    Hardware_SSD1306_Display("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmn");
    HAL_Delay(2000);
    Hardware_SSD1306_Display("Hardware_SSD1306_Display");
}
