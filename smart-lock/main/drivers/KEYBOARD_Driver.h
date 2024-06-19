#ifndef __KEYBOARD_H_
#define __KEYBOARD_H_

#include <inttypes.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 将有用的引脚使用宏定义来定义，增强代码的可读性
#define I2C_SCL 1
#define I2C_SDA 2
#define KEYBOARD_INT 0

// 设置sda的方向
#define I2C_SDA_IN gpio_set_direction(I2C_SDA, GPIO_MODE_INPUT)
#define I2C_SDA_OUT gpio_set_direction(I2C_SDA, GPIO_MODE_OUTPUT)

// 设置sda的电平
#define I2C_SDA_HIGH gpio_set_level(I2C_SDA, 1)
#define I2C_SDA_LOW gpio_set_level(I2C_SDA, 0)

// 设置scl的电平
#define I2C_SCL_HIGH gpio_set_level(I2C_SCL, 1)
#define I2C_SCL_LOW gpio_set_level(I2C_SCL, 0)

// 读取sda的引脚电平
#define I2C_READ_SDA gpio_get_level(I2C_SDA)

uint8_t KEYBOARD_read_key(void);

void KEYBOARD_Init(void);

#endif