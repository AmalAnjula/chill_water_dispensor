#include "max7219.h"

#define DIN_PIN   GPIO_PIN_7
#define DIN_PORT  GPIOA
 
#define CLK_PIN   GPIO_PIN_5
#define CLK_PORT  GPIOA

#define CS_PIN    GPIO_PIN_6
#define CS_PORT   GPIOA

static void MAX7219_WriteByte(uint8_t data)
{
    for(int i=0;i<8;i++)
    {
        HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_RESET);

        if(data & 0x80)
            HAL_GPIO_WritePin(DIN_PORT, DIN_PIN, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(DIN_PORT, DIN_PIN, GPIO_PIN_RESET);

        HAL_GPIO_WritePin(CLK_PORT, CLK_PIN, GPIO_PIN_SET);

        data <<= 1;
    }
}

void MAX7219_Send(uint8_t address, uint8_t data)
{
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);

    MAX7219_WriteByte(address);
    MAX7219_WriteByte(data);

    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

void MAX7219_Init(void)
{
    MAX7219_Send(0x09,0x00);   // No decode
    MAX7219_Send(0x0A,0x08);   // Intensity
    MAX7219_Send(0x0B,0x07);   // Scan all rows
    MAX7219_Send(0x0C,0x01);   // Normal operation
    MAX7219_Send(0x0F,0x00);   // Display test off

    MAX7219_Clear();
}

void MAX7219_Clear(void)
{
    for(uint8_t i=1;i<=8;i++)
        MAX7219_Send(i,0x00);
}