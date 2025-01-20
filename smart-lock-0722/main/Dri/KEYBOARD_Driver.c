#include "KEYBOARD_Driver.h"
#include "Utils.h"

#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include <stdint.h>

#define I2C_SDA GPIO_NUM_2
#define I2C_SCL GPIO_NUM_1

#define I2C_SDA_SEL (1ULL << I2C_SDA)
#define I2C_SCL_SEL (1ULL << I2C_SCL)
#define KEYBOARD_INT_SEL (1ULL << KEYBOARD_INT)

#define I2C_SDA_OUT gpio_set_direction(I2C_SDA, GPIO_MODE_OUTPUT)
#define I2C_SDA_IN gpio_set_direction(I2C_SDA, GPIO_MODE_INPUT)

#define I2C_SDA_HIGH gpio_set_level(I2C_SDA, 1)
#define I2C_SDA_LOW gpio_set_level(I2C_SDA, 0)

#define I2C_SCL_HIGH gpio_set_level(I2C_SCL, 1)
#define I2C_SCL_LOW gpio_set_level(I2C_SCL, 0)

#define I2C_SDA_READ gpio_get_level(I2C_SDA)

void I2C_Start(void) {
  I2C_SDA_OUT;
  I2C_SDA_HIGH;
  I2C_SCL_HIGH;
  DelayMs(1);
  I2C_SDA_LOW;
  DelayMs(1);
  I2C_SCL_LOW;
  DelayMs(1);
}

uint8_t I2C_SendByteAndGetNACK(uint8_t dataToSend) {
  uint8_t i;
  uint8_t ack;
  I2C_SDA_OUT;
  for (i = 0; i < 8; i++) {
    I2C_SCL_LOW;
    DelayMs(1);
    if ((dataToSend >> 7) & 0b00000001) {
      I2C_SDA_HIGH;
    } else {
      I2C_SDA_LOW;
    }
    DelayMs(1);
    I2C_SCL_HIGH;
    DelayMs(1);
    dataToSend <<= 1;
  }

  I2C_SCL_LOW;
  DelayMs(3);
  I2C_SDA_IN;
  DelayMs(3);
  I2C_SCL_HIGH;
  DelayMs(1);
  i = 250;
  while (i--) {
    if (I2C_SDA_READ == 0) {
      I2C_SCL_LOW;
      return 0;
    }
  }

  I2C_SCL_LOW;
  return 1;
}

void I2C_Respond(uint8_t ACKSignal) {
  I2C_SDA_OUT;
  I2C_SDA_LOW;
  I2C_SCL_LOW;
  if (ACKSignal) {
    I2C_SDA_HIGH;
  } else {
    I2C_SDA_LOW;
  }

  DelayMs(1);
  I2C_SCL_HIGH;
  DelayMs(1);
  I2C_SCL_LOW;
}

void I2C_Stop(void) {
  I2C_SCL_LOW;
  I2C_SDA_OUT;
  I2C_SDA_LOW;
  DelayMs(1);
  I2C_SCL_HIGH;
  DelayMs(1);
  I2C_SDA_HIGH;
}

uint8_t I2C_Receive8Bit(void) {
  uint8_t i = 0;
  uint8_t buffer = 0;
  I2C_SDA_IN;
  I2C_SCL_LOW;
  for (i = 0; i < 8; i++) {
    DelayMs(1);
    I2C_SCL_HIGH;
    buffer = (buffer << 1) | I2C_SDA_READ;
    DelayMs(1);
    I2C_SCL_LOW;
  }

  return buffer;
}

uint16_t I2C_SimpleReadFromDevice(uint8_t deviceAddr) {
  uint8_t buf1, buf2;

  I2C_Start();
  if (I2C_SendByteAndGetNACK((deviceAddr << 1) | 0b00000001)) {
    I2C_Stop();
    return 0xFFFF;
  }

  buf1 = I2C_Receive8Bit();
  I2C_Respond(0);
  buf2 = I2C_Receive8Bit();
  I2C_Respond(1);
  I2C_Stop();
  return ((uint16_t)buf1 << 8) | buf2;
}

void KEYBOARD_Init(void) {
  gpio_config_t io_conf = {};
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.pin_bit_mask = I2C_SDA_SEL | I2C_SCL_SEL;
  gpio_config(&io_conf);

  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.intr_type = GPIO_INTR_POSEDGE;
  io_conf.pin_bit_mask = KEYBOARD_INT_SEL;
  gpio_config(&io_conf);
}

uint16_t KEYBOARD_ReadKey(void) {
  uint16_t content = I2C_SimpleReadFromDevice(0x42);
  if (content == 0x8000)
    return 0;
  if (content == 0x4000)
    return 1;
  if (content == 0x2000)
    return 2;
  if (content == 0x1000)
    return 3;
  if (content == 0x100)
    return 4;
  if (content == 0x400)
    return 5;
  if (content == 0x200)
    return 6;
  if (content == 0x800)
    return 7;
  if (content == 0x40)
    return 8;
  if (content == 0x20)
    return 9;
  if (content == 0x10)
    return '#';
  if (content == 0x80)
    return 'M';
  return 0xFFFF;
}