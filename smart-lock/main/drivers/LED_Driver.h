#ifndef __LED_H_
#define __LED_H_

#include <stdint.h>
#include "driver/rmt_encoder.h"
#include "esp_check.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      6

/// led灯珠的数量
#define LED_NUMBERS         12
#define CHASE_SPEED_MS      10

/**
 * @brief Type of led strip encoder configuration
 */
typedef struct
{
    uint32_t resolution; /*!< Encoder resolution, in Hz */
} led_strip_encoder_config_t;

void LED_Init(void);
void light_led(uint8_t);

#endif