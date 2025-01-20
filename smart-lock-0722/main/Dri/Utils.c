#include "Utils.h"
#include "freertos/FreeRTOS.h"
#include <unistd.h>

void DelayMs(uint32_t time) { vTaskDelay(time / portTICK_PERIOD_MS); }

void DelayUs(uint32_t time) { usleep(time); }