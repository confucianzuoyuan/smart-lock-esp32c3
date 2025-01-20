#pragma once

#include "stdint.h"

#define KEYBOARD_INT GPIO_NUM_0

void KEYBOARD_Init(void);
uint16_t KEYBOARD_ReadKey(void);