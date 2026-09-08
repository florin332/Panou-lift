#ifndef __BSP_I2C_H__
#define __BSP_I2C_H__

#include <stdint.h>
#include <stddef.h>

// Waveshare RP2350 Touch LCD 2.8
#define BSP_I2C_NUM      i2c1
#define BSP_I2C_SDA_PIN  6
#define BSP_I2C_SCL_PIN  7

void bsp_i2c_write(uint8_t device_addr, uint8_t *buffer, size_t len);

void bsp_i2c_write_reg8(
    uint8_t device_addr,
    uint8_t reg_addr,
    uint8_t *buffer,
    size_t len);

void bsp_i2c_read_reg8(
    uint8_t device_addr,
    uint8_t reg_addr,
    uint8_t *buffer,
    size_t len);

void bsp_i2c_write_reg16(
    uint8_t device_addr,
    uint16_t reg_addr,
    uint8_t *buffer,
    size_t len);

void bsp_i2c_read_reg16(
    uint8_t device_addr,
    uint16_t reg_addr,
    uint8_t *buffer,
    size_t len);

void bsp_i2c_init(void);

#endif /* __BSP_I2C_H__ */