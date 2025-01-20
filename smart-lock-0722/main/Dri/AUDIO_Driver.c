#include "AUDIO_Driver.h"

#include "Utils.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"

void AUDIO_Init(void) {
  gpio_config_t io_conf = {};
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.pin_bit_mask = (1ULL << 9);
  gpio_config(&io_conf);

  // 拉高标识传输数据结束
  gpio_set_level(9, 1);
  DelayMs(100);
}

void AUDIO_Play(uint8_t value) {
  // 拉低10ms，标识要开始传输数据
  gpio_set_level(9, 0);
  DelayMs(10);
  for (int i = 0; i < 8; i++) {
    if (value & 0b00000001) {
      gpio_set_level(9, 1);
      DelayUs(600);
      gpio_set_level(9, 0);
      DelayUs(200);
    } else {
      gpio_set_level(9, 1);
      DelayUs(200);
      gpio_set_level(9, 0);
      DelayUs(600);
    }
    value = value >> 1;
  }

  // 拉高标识传输数据结束
  gpio_set_level(9, 1);
  DelayMs(100);
}