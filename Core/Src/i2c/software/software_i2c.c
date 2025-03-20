#include "main.h"
#include "stm32f1xx_hal_gpio.h"
#include "error.h"
#include "DEMCR/DWT.h"
#include "i2c/SSD1306.h"

#define SIM_I2C_SCL_PIN   GPIO_PIN_12
#define SIM_I2C_SDA_PIN   GPIO_PIN_5
#define SIM_I2C_GPIO_PORT GPIOA

static uint8_t DELAY_IN_US = 1;

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

void SIM_I2C_Start(void) {
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SDA_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SCL_PIN, GPIO_PIN_SET);
    DWT_DelayUs(DELAY_IN_US);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SDA_PIN, GPIO_PIN_RESET);
    DWT_DelayUs(DELAY_IN_US);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SCL_PIN, GPIO_PIN_RESET);
}

void SIM_I2C_Stop(void) {
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SDA_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SCL_PIN, GPIO_PIN_SET);
    DWT_DelayUs(DELAY_IN_US);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SDA_PIN, GPIO_PIN_SET);
    DWT_DelayUs(DELAY_IN_US);
}

void SIM_I2C_SendBit(const uint8_t bit) {
    if (bit)
        HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SDA_PIN, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SDA_PIN, GPIO_PIN_RESET);

    DWT_DelayUs(DELAY_IN_US);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SCL_PIN, GPIO_PIN_SET);
    DWT_DelayUs(DELAY_IN_US);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SCL_PIN, GPIO_PIN_RESET);
    DWT_DelayUs(DELAY_IN_US);
}

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
    DWT_DelayUs(DELAY_IN_US);
    const uint8_t ack = HAL_GPIO_ReadPin(SIM_I2C_GPIO_PORT, SIM_I2C_SDA_PIN);
    HAL_GPIO_WritePin(SIM_I2C_GPIO_PORT, SIM_I2C_SCL_PIN, GPIO_PIN_RESET);
    DWT_DelayUs(DELAY_IN_US);

    /* 恢复 SDA 为输出模式 */
    GPIO_InitStruct.Pin = SIM_I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SIM_I2C_GPIO_PORT, &GPIO_InitStruct);

    return ack; // 返回 0 表示收到 ACK，否则未收到 ACK
}

void SIM_I2C_SendByteWithCheck(const uint8_t byte, const char *file, const int line, const char *errorMsg) {
    if (SIM_I2C_SendByte(byte) != 0) {
        SIM_I2C_Stop();
        Error_Handler_UART(file, line, errorMsg);
    }
}

void SendCommand(const uint8_t cmd) {
    SIM_I2C_Start();
    // 发送设备地址（写模式，最低位为0）
    SIM_I2C_SendByteWithCheck(SSD1306_I2C_ADDR | 0,__FILE__, __LINE__, "Send addr failed.\r\n");
    SIM_I2C_SendByteWithCheck(SSD1306_CMD_CTRL_BYTE,__FILE__, __LINE__, "Send cmd crl byte failed.\r\n");
    SIM_I2C_SendByteWithCheck(cmd,__FILE__, __LINE__, "Send cmd failed.\r\n");
    SIM_I2C_Stop();
}

void SendData(const uint8_t data) {
    SIM_I2C_Start();
    SIM_I2C_SendByteWithCheck(SSD1306_I2C_ADDR | 0,__FILE__, __LINE__, "Send addr failed.\r\n");
    SIM_I2C_SendByteWithCheck(SSD1306_DATA_CTRL_BYTE,__FILE__, __LINE__, "Send data crl byte failed.\r\n");
    SIM_I2C_SendByteWithCheck(data,__FILE__, __LINE__, "Send data failed.\r\n");
    SIM_I2C_Stop();
}

void SendDataBulk(const uint8_t *data, const uint16_t len) {
    SIM_I2C_Start();
    SIM_I2C_SendByteWithCheck(SSD1306_I2C_ADDR | 0,__FILE__, __LINE__, "Send addr failed.\r\n");
    SIM_I2C_SendByteWithCheck(SSD1306_DATA_CTRL_BYTE,__FILE__, __LINE__, "Send data crl byte failed.\r\n");
    for (uint16_t i = 0; i < len; i++) {
        SIM_I2C_SendByteWithCheck(data[i],__FILE__, __LINE__, "Send data failed.\r\n");
    }
    SIM_I2C_Stop();
}
