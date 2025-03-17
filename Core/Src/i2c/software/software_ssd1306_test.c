#include "i2c/software/software_ssd1306.h"

void Software_SSD1306_Init_Test(const uint8_t width_input, const uint8_t height_input) {
    Software_SSD1306_Init(width_input, height_input);
    HAL_Delay(2000);
    Software_SSD1306_Display("hello world.");
    HAL_Delay(2000);
    Software_SSD1306_Display("Software_SSD1306_Display");
}
