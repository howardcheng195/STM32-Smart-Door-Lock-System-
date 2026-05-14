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

static void Keypad_AllRowsHigh(void)
{
    for (int i = 0; i < 4; i++) {
        HAL_GPIO_WritePin(row_ports[i], row_pins[i], GPIO_PIN_SET);
    }
}

static char Keypad_ScanOnce(void)
{
    char detected_key = 0;
    int key_count = 0;

    for (int row = 0; row < 4; row++) {

        Keypad_AllRowsHigh();

        HAL_GPIO_WritePin(row_ports[row], row_pins[row], GPIO_PIN_RESET);

        HAL_Delay(1);

        for (int col = 0; col < 4; col++) {
            if (HAL_GPIO_ReadPin(col_ports[col], col_pins[col]) == GPIO_PIN_RESET) {
                detected_key = keymap[row][col];
                key_count++;
            }
        }
    }

    Keypad_AllRowsHigh();
    // 多鍵同時按下，忽略，避免 ghosting
    if (key_count != 1) {
        return 0;
    }

    return detected_key;
}

static char last_key = 0;
static char debounce_key = 0;
static uint32_t debounce_time = 0;

// non-blocking
char Keypad_GetKey(void)
{
    char key = Keypad_ScanOnce();

    // 沒按鍵，代表已放開
    if (key == 0) {
        last_key = 0;
        debounce_key = 0;
        debounce_time = 0;
        return 0;
    }

    // 按鍵剛變化，開始 debounce 計時
    if (key != debounce_key) {
        debounce_key = key;
        debounce_time = HAL_GetTick();
        return 0;
    }

    // 還沒穩定 20ms，不輸出
    if (HAL_GetTick() - debounce_time < 20) {
        return 0;
    }

    // 如果還是上一顆按住，不重複回傳
    if (key == last_key) {
        return 0;
    }

    // 新按鍵
    last_key = key;
    return key;
}

