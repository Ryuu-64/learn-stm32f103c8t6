#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "i2c/font.h"
#include "i2c/software/software_ssd1306.h"
#include "i2c/software/software_i2c.h"

static uint8_t *buffer;
static uint8_t width_in_pixel, height_in_pixel;

/* 更新屏幕，将 buffer 中的数据写入 OLED 显示屏（按页写入） */
static void UpdateScreen(void) {
    for (uint8_t page = 0; page < 8; page++) {
        // 设置页地址（0xB0 ~ 0xB7）
        SendCommand(0xB0 + page);
        // 这里假设列地址起始为 0
        // 你也可以调用 SetPosition(x, y) 来设置列地址
        // 发送当前页的所有列数据
        SendDataBulk(&buffer[page * width_in_pixel], width_in_pixel);
    }
}

/* 填充显示缓冲区 */
static void Fill(const uint8_t color) {
    memset(buffer, color, width_in_pixel * height_in_pixel);
}

static void SetPosition(const uint8_t x, const uint8_t y) {
    // 设置页地址（y 的值应在 0 ~ 7 之间）
    SendCommand(0xB0 + y);
    // 设置列地址低位，x 的低4位
    SendCommand(0x00 + (x & 0x0F));
    // 设置列地址高位，x 的高4位
    SendCommand(0x10 + (x >> 4 & 0x0F));
}

/* 显示单个字符（8x16 字体） */
static void DisplayChar(const uint8_t ch, const uint8_t x, const uint8_t y) {
    const uint32_t ascii_code = ch + ASCII_OFFSET;
    // 显示字符上半部分
    SetPosition(x, y);
    for (uint8_t i = 0; i < 8; i++) {
        SendData(ascii_code_8x16[ascii_code][i]);
    }
    // 显示字符下半部分
    SetPosition(x, y + 1);
    for (uint8_t i = 0; i < 8; i++) {
        SendData(ascii_code_8x16[ascii_code][i + 8]);
    }
}

/* 依次显示字符串 */
void Software_SSD1306_Display(const uint8_t *chars) {
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

/* 初始化 SSD1306，使用模拟 I2C 实现 */
void Software_SSD1306_Init(const uint8_t width_input, const uint8_t height_input) {
    SIM_I2C_GPIO_Init();

    width_in_pixel = width_input;
    height_in_pixel = height_input;
    buffer = calloc(width_in_pixel * height_in_pixel, sizeof(uint8_t));
    if (buffer == NULL) {
        Error_Handler_UART(__FILE__, __LINE__, "buffer == NULL.\n");
    }

    // 初始化命令序列（参考 SSD1306 datasheet，根据实际硬件调整）
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
