/*
 * bluetooth.c
 *
 *  Created on: Mar 27, 2026
 *      Author: 豪
 */
#include "bluetooth.h"
#include <string.h>

static UART_HandleTypeDef *bt_uart = NULL;

void BT_Init(UART_HandleTypeDef *huart)
{
    bt_uart = huart;
}

void BT_Send(const char *msg)
{
    if (bt_uart == NULL || msg == NULL)
    {
        return;
    }

    HAL_UART_Transmit(bt_uart, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}


