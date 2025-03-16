#ifndef SOFTWARE_I2C_H
#define SOFTWARE_I2C_H

void SIM_I2C_GPIO_Init(void);

void SendCommand(uint8_t cmd);

void SendData(uint8_t data);

void SendDataBulk(const uint8_t *data, uint16_t len);

#endif //SOFTWARE_I2C_H
