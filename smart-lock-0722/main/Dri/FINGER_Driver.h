#pragma once

#include <stdint.h>

#define FINGER_INT 10

void FINGER_Init(void);
void FINGER_ReadSerialNumber(void);
uint8_t FINGER_Sleep(void);
uint8_t FINGER_GetTemplatesNumber(void);
uint8_t FINGER_GetImage(void);
uint8_t FINGER_GenChar(uint8_t BufferID);
uint8_t FINGER_Search(void);
void FINGER_SetSecurityToZero(void);
uint8_t FINGER_Enroll(uint8_t PageID);
uint8_t FINGER_Identify(void);