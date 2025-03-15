#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "i2c/font.h"
#include "i2c/software/software_ssd1306.h"
#include "DEMCR/DWT.h"

#define SSD1306_I2C_ADDR        0x3C << 1
#define SSD1306_CMD_CTRL_BYTE   0x00
#define SSD1306_DATA_CTRL_BYTE  0x40

static uint8_t *buffer;
static uint8_t width_in_pixel, height_in_pixel;

/* 定义模拟 I2C 的引脚
   这里假设使用 GPIOB 的 PB6 作为 SCL，PB7 作为 SDA */
#define SIM_I2C_SCL_PIN   GPIO_PIN_6
#define SIM_I2C_SDA_PIN   GPIO_PIN_7
#define SIM_I2C_GPIO_PORT GPIOB

static uint8_t DWT_DELAY_US = 1;

/* 初始化 GPIO 模拟 I2C */
void SIM_I2C_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* 配置 SCL 和 SDA 引脚为推挽输出 */
    GPIO_InitStruct.Pin = SIM_I2C_SCL_PIN | SIM_I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; // 开漏输出
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SIM_I2C_GPIO_PORT, &GPIO_InitStruct);

    /* 设置 SCL 和 SDA 默认高电平（空闲状态） */
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SCL_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SDA_PIN, GPIO_PIN_SET);
}

/* 模拟 I2C 启动信号：SDA 从高拉到低，而 SCL 保持高 */
void SIM_I2C_Start(void) {
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SDA_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SCL_PIN, GPIO_PIN_SET);
    DWT_DelayUs(DWT_DELAY_US);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SDA_PIN, GPIO_PIN_RESET);
    DWT_DelayUs(DWT_DELAY_US);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SCL_PIN, GPIO_PIN_RESET);
}

/* 模拟 I2C 停止信号：SDA 从低拉到高，SCL 先保持高 */
void SIM_I2C_Stop(void) {
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SDA_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SCL_PIN, GPIO_PIN_SET);
    DWT_DelayUs(DWT_DELAY_US);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SDA_PIN, GPIO_PIN_SET);
    DWT_DelayUs(DWT_DELAY_US);
}

/* 发送一位数据 */
void SIM_I2C_SendBit(const uint8_t bit) {
    if (bit)
        HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SDA_PIN, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SDA_PIN, GPIO_PIN_RESET);

    DWT_DelayUs(DWT_DELAY_US);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SCL_PIN, GPIO_PIN_SET);
    DWT_DelayUs(DWT_DELAY_US);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SCL_PIN, GPIO_PIN_RESET);
    DWT_DelayUs(DWT_DELAY_US);
}

/* 发送一个字节，并接收 ACK 信号 */
uint8_t SIM_I2C_SendByte(const uint8_t byte) {
    for (int i = 7; i >= 0; i--) {
        SIM_I2C_SendBit(byte >> i & 0x01);
    }

    /* 配置 SDA 为输入模式以接收 ACK */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SIM_I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SIM_I2C_GPIO_PORT, &GPIO_InitStruct);

    /* 发送 ACK 时 SCL 上升 */
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SCL_PIN, GPIO_PIN_SET);
    DWT_DelayUs(DWT_DELAY_US);
    uint8_t ack = HAL_GPIO_ReadPin(SIM_I2C_GPIO_PORT, SIM_I2C_SDA_PIN);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SCL_PIN, GPIO_PIN_RESET);
    DWT_DelayUs(DWT_DELAY_US);

    /* 恢复 SDA 为输出模式 */
    GPIO_InitStruct.Pin = SIM_I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SIM_I2C_GPIO_PORT, &GPIO_InitStruct);

    return ack; // 返回 0 表示收到 ACK，否则未收到 ACK
}

/* 使用模拟 I2C 发送命令 */
static void SendCommand(const uint8_t cmd) {
    SIM_I2C_Start();
    // 发送设备地址（写模式，最低位为0）
    if (SIM_I2C_SendByte(SSD1306_I2C_ADDR | 0) != 0) {
        SIM_I2C_Stop();
        Error_Handler_UART(__FILE__, __LINE__, "SIM_I2C_SendByte(SSD1306_I2C_ADDR | 0) != 0");
    }
    // 发送控制字节：0x00 表示后续为命令
    if (SIM_I2C_SendByte(SSD1306_CMD_CTRL_BYTE) != 0) {
        SIM_I2C_Stop();
        Error_Handler_UART(__FILE__, __LINE__, "SIM_I2C_SendByte(SSD1306_CMD_CTRL_BYTE) != 0");
    }
    // 发送命令字节
    if (SIM_I2C_SendByte(cmd) != 0) {
        SIM_I2C_Stop();
        Error_Handler_UART(__FILE__, __LINE__, "SIM_I2C_SendByte(cmd) != 0");
    }
    SIM_I2C_Stop();
}

/* 使用模拟 I2C 发送单个数据字节 */
static void SendData(const uint8_t data) {
    SIM_I2C_Start();
    if (SIM_I2C_SendByte(SSD1306_I2C_ADDR | 0) != 0) {
        SIM_I2C_Stop();
        Error_Handler_UART(__FILE__, __LINE__, "SIM_I2C_SendByte(SSD1306_I2C_ADDR | 0) != 0");
    }
    // 发送控制字节：0x40 表示后续为数据
    if (SIM_I2C_SendByte(SSD1306_DATA_CTRL_BYTE) != 0) {
        SIM_I2C_Stop();
        Error_Handler_UART(__FILE__, __LINE__, "SIM_I2C_SendByte(SSD1306_DATA_CTRL_BYTE) != 0");
    }
    // 发送数据字节
    if (SIM_I2C_SendByte(data) != 0) {
        SIM_I2C_Stop();
        Error_Handler_UART(__FILE__, __LINE__, "SIM_I2C_SendByte(data) != 0");
    }
    SIM_I2C_Stop();
}

/* 使用模拟 I2C 批量发送数据 */
static void SendDataBulk(const uint8_t *data, const uint16_t len) {
    SIM_I2C_Start();
    if (SIM_I2C_SendByte(SSD1306_I2C_ADDR | 0) != 0) {
        SIM_I2C_Stop();
        Error_Handler_UART(__FILE__, __LINE__, "SIM_I2C_SendByte(SSD1306_I2C_ADDR | 0) != 0");
    }
    // 发送控制字节：0x40 表示数据模式
    if (SIM_I2C_SendByte(SSD1306_DATA_CTRL_BYTE) != 0) {
        SIM_I2C_Stop();
        Error_Handler_UART(__FILE__, __LINE__, "SIM_I2C_SendByte(SSD1306_DATA_CTRL_BYTE) != 0");
    }
    for (uint16_t i = 0; i < len; i++) {
        if (SIM_I2C_SendByte(data[i]) != 0) {
            SIM_I2C_Stop();
            Error_Handler_UART(__FILE__, __LINE__, "SIM_I2C_SendByte(data[i]) != 0");
        }
    }
    SIM_I2C_Stop();
}

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
