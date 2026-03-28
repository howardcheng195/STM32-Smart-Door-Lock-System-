#include "lcd_i2c.h"
#include <string.h>

#define LCD_I2C_ADDR     (0x27 << 1)   // STM32 HAL 用 8-bit address
#define LCD_BACKLIGHT    0x08
#define LCD_ENABLE       0x04
#define LCD_RW           0x02
#define LCD_RS           0x01

static I2C_HandleTypeDef *lcd_i2c;

static void LCD_Write4Bits(uint8_t data);
static void LCD_SendInternal(uint8_t data, uint8_t flags);
static void LCD_SendCommand(uint8_t cmd);
static void LCD_SendData(uint8_t data);

static void LCD_Write4Bits(uint8_t data)
{
    uint8_t data_t[1];
    data_t[0] = data | LCD_BACKLIGHT;
    HAL_I2C_Master_Transmit(lcd_i2c, LCD_I2C_ADDR, data_t, 1, HAL_MAX_DELAY);
}

static void LCD_PulseEnable(uint8_t data)
{
    LCD_Write4Bits(data | LCD_ENABLE);
    HAL_Delay(1);
    LCD_Write4Bits(data & ~LCD_ENABLE);
    HAL_Delay(1);
}

static void LCD_SendInternal(uint8_t data, uint8_t flags)
{
    uint8_t high = (data & 0xF0) | flags;
    uint8_t low  = ((data << 4) & 0xF0) | flags;

    LCD_Write4Bits(high);
    LCD_PulseEnable(high);

    LCD_Write4Bits(low);
    LCD_PulseEnable(low);
}

static void LCD_SendCommand(uint8_t cmd)
{
    LCD_SendInternal(cmd, 0x00);
}

static void LCD_SendData(uint8_t data)
{
    LCD_SendInternal(data, LCD_RS);
}

void LCD_Init(I2C_HandleTypeDef *hi2c)
{
    lcd_i2c = hi2c;
    HAL_Delay(50);

    // 初始化成 4-bit mode
    LCD_Write4Bits(0x30);
    LCD_PulseEnable(0x30);
    HAL_Delay(5);

    LCD_Write4Bits(0x30);
    LCD_PulseEnable(0x30);
    HAL_Delay(1);

    LCD_Write4Bits(0x30);
    LCD_PulseEnable(0x30);
    HAL_Delay(10);

    LCD_Write4Bits(0x20);
    LCD_PulseEnable(0x20);
    HAL_Delay(10);

    // Function set: 4-bit, 2 line, 5x8
    LCD_SendCommand(0x28);

    // Display ON, Cursor OFF, Blink OFF
    LCD_SendCommand(0x0C);

    // Entry mode
    LCD_SendCommand(0x06);

    // Clear display
    LCD_SendCommand(0x01);
    HAL_Delay(2);
}

void LCD_Clear(void)
{
    LCD_SendCommand(0x01);
    HAL_Delay(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t address;

    if (row == 0)
        address = 0x80 + col;
    else
        address = 0xC0 + col;

    LCD_SendCommand(address);
}

void LCD_Print(char *str)
{
    while (*str)
    {
        LCD_SendData((uint8_t)(*str));
        str++;
    }
}

void LCD_PrintLine(uint8_t row, char *str)
{
    LCD_SetCursor(row, 0);
    LCD_Print("                ");   // 先清整行
    LCD_SetCursor(row, 0);
    LCD_Print(str);
}
