#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "i2c/font.h"
#include "i2c/hardware/hardware_ssd1306.h"
#include "i2c/SSD1306.h"
#include "error.h"

static I2C_HandleTypeDef *hi2c;
static uint8_t *buffer;
static uint8_t width_in_pixel, height_in_pixel;

static void SendCommand(const uint8_t cmd) {
    uint8_t buf[2] = {SSD1306_CMD_CTRL_BYTE, cmd};
    const HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        hi2c, SSD1306_I2C_ADDR, buf, sizeof(buf), 100
    );
    if (status == HAL_OK) {
        return;
    }

    Error_Handler_UART(__FILE__, __LINE__, "SendCommand failed. HAL_StatusTypeDef = %d.\r\n", status);
}

static void SetPosition(const uint8_t x, const uint8_t y) {
    // 设置页地址（y 的值应在 0 ~ 7 之间）
    SendCommand(0xB0 + y);
    // 设置列地址低位，x 的低4位
    SendCommand(0x00 + (x & 0x0F));
    // 设置列地址高位，x 的高4位
    SendCommand(0x10 + (x >> 4 & 0x0F));
}

static void SendData(const uint8_t data) {
    uint8_t buf[2] = {SSD1306_DATA_CTRL_BYTE, data};
    const HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        hi2c, SSD1306_I2C_ADDR, buf, sizeof(buf), 100
    );
    if (status == HAL_OK) {
        return;
    }

    Error_Handler_UART(__FILE__, __LINE__, "SendData failed.");
}

static void SendDataBulk(const uint8_t *data, const uint16_t len) {
    uint8_t *buf = malloc(len + 1);
    buf[0] = SSD1306_DATA_CTRL_BYTE;
    memcpy(&buf[1], data, len);
    const HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        hi2c, SSD1306_I2C_ADDR, buf, len + 1, 100
    );
    free(buf);
    if (status == HAL_OK) {
        return;
    }

    Error_Handler_UART(__FILE__, __LINE__, "SendDataBulk failed.");
}

static void UpdateScreen(void) {
    for (uint8_t page = 0; page < 8; page++) {
        SendCommand(0xB0 + page);
        SendDataBulk(&buffer[page * width_in_pixel], width_in_pixel);
    }
}

static void Fill(const uint8_t color) {
    memset(buffer, color, width_in_pixel * height_in_pixel);
}

static void DisplayChar(const uint8_t chars, const uint8_t x, const uint8_t y) {
    const uint32_t ascii_code = chars + ASCII_OFFSET;
    SetPosition(x, y);
    for (uint8_t i = 0; i < 8; i++) {
        SendData(ascii_code_8x16[ascii_code][i]);
    }

    SetPosition(x, y + 1);
    for (uint8_t i = 0; i < 8; i++) {
        SendData(ascii_code_8x16[ascii_code][i + 8]);
    }
}

void Hardware_SSD1306_Display(const uint8_t *chars) {
    Fill(0x00);
    UpdateScreen();
    const uint8_t max_char_x = width_in_pixel / 8;
    const uint8_t max_char_y = height_in_pixel / 8;
    for (uint8_t y = 0; y < max_char_y; y += 2) {
        for (uint8_t x = 0; x < max_char_x; x++) {
            if (*chars == '\0') {
                return;
            }

            DisplayChar(*chars, x * 8, y);
            chars++;
        }
    }
}

void Hardware_SSD1306_Init(I2C_HandleTypeDef *hi2c_input, const uint8_t width_input, const uint8_t height_input) {
    hi2c = hi2c_input;
    width_in_pixel = width_input;
    height_in_pixel = height_input;
    buffer = calloc(width_in_pixel * height_in_pixel, sizeof(uint8_t));
    if (buffer == NULL) {
        Error_Handler_UART(__FILE__, __LINE__, "buffer == NULL.\n");
    }

    SendCommand(0xAE); // 关闭显示

    SendCommand(0x20); // 设置内存寻址模式
    SendCommand(0x02); // 页寻址模式

    SendCommand(0xA1); // 段重映射：列0 -> SEG0（正常方向）
    SendCommand(0xC8); // COM扫描方向：正常（从上到下）

    SendCommand(0x40); // 设置显示起始行 = 行0

    SendCommand(0x81); // 设置对比度
    SendCommand(0xFF); // 最大亮度

    SendCommand(0xA6); // 正常显示（0xA7为反色）

    SendCommand(0xA8); // 设置复用率
    SendCommand(0x3F); // 1/64 Duty（64行）

    SendCommand(0xD3); // 设置显示偏移
    SendCommand(0x00); // 无偏移

    SendCommand(0xD5); // 设置显示时钟分频
    SendCommand(0xF0); // 分频比=1，振荡频率=0xF

    SendCommand(0xD9); // 设置预充电周期
    SendCommand(0x22); // Phase1=2 DCLK, Phase2=2 DCLK

    SendCommand(0xDA); // 设置COM引脚配置
    SendCommand(0x12); // Sequential模式（64行）

    SendCommand(0xDB); // 设置VCOMH电压
    SendCommand(0x20); // 0.77×VCC

    SendCommand(0x8D); // 电荷泵设置
    SendCommand(0x14); // 启用电荷泵（必须！）

    SendCommand(0xAF); // 开启显示

    Fill(0x00);
    UpdateScreen();
}
