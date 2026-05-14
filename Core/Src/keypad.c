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

// 紀錄上一個按鈕
static char last_key = 0;
// 避免先按下一個按鈕不放開，又按下一個
static uint8_t key_locked = 0;

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

static char Keypad_ScanOnce(int *key_count_out)
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

    *key_count_out = key_count;

    // 一次只能按一個，避免ghosting
    if (key_count != 1) {
        return 0;
    }

    return detected_key;
}

char Keypad_GetKey(void)
{
    int count1 = 0;
    char key1 = Keypad_ScanOnce(&count1);

    // 沒有按鍵，解除鎖定
    if (count1 == 0) {
        last_key = 0;
        key_locked = 0;
        return 0;
    }

    // 多鍵狀態，鎖住直到全部放開
    if (count1 > 1) {
        key_locked = 1;
        return 0;
    }

    // 如果之前進入多鍵狀態，必須等全部放開才接受新鍵
    if (key_locked) {
        return 0;
    }

    HAL_Delay(20);

    int count2 = 0;
    char key2 = Keypad_ScanOnce(&count2);

    if (count2 == 0) {
        last_key = 0;
        key_locked = 0;
        return 0;
    }

    // debounce 後變成多鍵，鎖住直到全部放開
    if (count2 > 1) {
        key_locked = 1;
        return 0;
    }

    // debounce 後變成另一顆鍵，忽略
    if (key1 != key2) {
        return 0;
    }

    if (key1 == last_key) {
        return 0;
    }

    last_key = key1;
    return key1;
}
