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
    char detected_key = 0;
    int key_count = 0;

    for (int row = 0; row < 4; row++) {

        // 全部 row 拉高
        for (int i = 0; i < 4; i++) {
            HAL_GPIO_WritePin(row_ports[i], row_pins[i], GPIO_PIN_SET);
        }

        // 目前 row 拉低
        HAL_GPIO_WritePin(row_ports[row], row_pins[row], GPIO_PIN_RESET);

        HAL_Delay(1);

        for (int col = 0; col < 4; col++) {
            if (HAL_GPIO_ReadPin(col_ports[col], col_pins[col]) == GPIO_PIN_RESET) {
                detected_key = keymap[row][col];
                key_count++;
            }
        }
    }

    // 掃描完 row 全部拉高
    for (int i = 0; i < 4; i++) {
        HAL_GPIO_WritePin(row_ports[i], row_pins[i], GPIO_PIN_SET);
    }

    // 沒有按鍵
    if (key_count == 0) {
        return 0;
    }

    // 多鍵同時按下，忽略，避免 ghosting
    if (key_count > 1) {
        return 0;
    }

    // debounce：等一下再確認一次
    HAL_Delay(20);

    // 簡單確認是否仍有按鍵
    int still_pressed = 0;

    for (int row = 0; row < 4; row++) {
        for (int i = 0; i < 4; i++) {
            HAL_GPIO_WritePin(row_ports[i], row_pins[i], GPIO_PIN_SET);
        }

        HAL_GPIO_WritePin(row_ports[row], row_pins[row], GPIO_PIN_RESET);
        HAL_Delay(1);

        for (int col = 0; col < 4; col++) {
            if (HAL_GPIO_ReadPin(col_ports[col], col_pins[col]) == GPIO_PIN_RESET) {
                still_pressed++;
            }
        }
    }

    for (int i = 0; i < 4; i++) {
        HAL_GPIO_WritePin(row_ports[i], row_pins[i], GPIO_PIN_SET);
    }

    if (still_pressed == 1) {
        return detected_key;
    }

    return 0;
}
