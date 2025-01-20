#include "MOTOR_Driver.h"

#include "Utils.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"

void MOTOR_Init(void) {
  gpio_config_t io_conf = {};
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.pin_bit_mask = (1ULL << 4) | (1ULL << 5);
  gpio_config(&io_conf);

  gpio_set_level(4, 0);
  gpio_set_level(5, 0);
}

void MOTOR_OpenLock(void) {
  gpio_set_level(4, 1);
  gpio_set_level(5, 0);
  DelayMs(2000);

  gpio_set_level(4, 0);
  gpio_set_level(5, 0);
  DelayMs(2000);

  gpio_set_level(4, 0);
  gpio_set_level(5, 1);
  DelayMs(2000);

  gpio_set_level(4, 0);
  gpio_set_level(5, 0);
}