/*
 * bluetooth.h
 *
 *  Created on: Mar 27, 2026
 *      Author: 豪
 */

#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "main.h"

void BT_Init(UART_HandleTypeDef *huart);
void BT_Send(const char *msg);

#endif
