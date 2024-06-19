#ifndef __AUDIO_H_
#define __AUDIO_H_

#include "driver/gpio.h"
#include "sys/unistd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/// 微秒延时
#define DELAY_US(i) usleep(i)
/// 毫秒延时
#define DELAY_MS(i) vTaskDelay(i / portTICK_PERIOD_MS)

#define AUDIO_SDA GPIO_NUM_9
#define AUDIO_SDA_HIGH gpio_set_level(AUDIO_SDA, 1)
#define AUDIO_SDA_LOW  gpio_set_level(AUDIO_SDA, 0)

void AUDIO_Send(uint8_t);
void AUDIO_Test(void);
void AUDIO_Init(void);

#endif