#ifndef __MOTOR_DRIVER_H_
#define __MOTOR_DRIVER_H_

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MOTOR_DRIVER_NUM_0 GPIO_NUM_4
#define MOTOR_DRIVER_NUM_1 GPIO_NUM_5

void MOTOR_Init(void);
void MOTOR_Open_lock(void);

#endif