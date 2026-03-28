/*
 * keypad.c
 *
 *  Created on: Mar 19, 2026
 *      Author: 豪
 */

#include "keypad.h"

static const char keymap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

static GPIO_TypeDef* row_ports[4] = {GPIOC, GPIOC, GPIOC, GPIOC};
static uint16_t row_pins[4] = {
    GPIO_PIN_7, GPIO_PIN_6, GPIO_PIN_5, GPIO_PIN_4
};

static GPIO_TypeDef* col_ports[4] = {GPIOC, GPIOC, GPIOC, GPIOC};
static uint16_t col_pins[4] = {
    GPIO_PIN_3, GPIO_PIN_2, GPIO_PIN_1, GPIO_PIN_0
};

void Keypad_Init(void)
{
	// row 拉高
    for (int i = 0; i < 4; i++) {
        HAL_GPIO_WritePin(row_ports[i], row_pins[i], GPIO_PIN_SET);
    }
}

char Keypad_GetKey(void)
{
    for (int row = 0; row < 4; row++) {

        // 先全部拉高
        for (int i = 0; i < 4; i++) {
            HAL_GPIO_WritePin(row_ports[i], row_pins[i], GPIO_PIN_SET);
        }

        // 只把目前這一列拉低
        HAL_GPIO_WritePin(row_ports[row], row_pins[row], GPIO_PIN_RESET);

        HAL_Delay(1);

        // 讀4個column
        for (int col = 0; col < 4; col++) {
            if (HAL_GPIO_ReadPin(col_ports[col], col_pins[col]) == GPIO_PIN_RESET) {

                // 等按鍵放開，避免一直重複觸發
                while (HAL_GPIO_ReadPin(col_ports[col], col_pins[col]) == GPIO_PIN_RESET);

                HAL_Delay(20); // debounce
                return keymap[row][col];
            }
        }
    }

    return 0;
}
