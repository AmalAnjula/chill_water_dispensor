#ifndef __MAX7219_H
#define __MAX7219_H

#include "main.h"

void MAX7219_Init(void);
void MAX7219_Send(uint8_t address, uint8_t data);
void MAX7219_Clear(void);
void MAX7219_DisplayDigit(uint8_t digit);

#endif