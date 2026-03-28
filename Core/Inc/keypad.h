/*
 * keypad.h
 *
 *  Created on: Mar 19, 2026
 *      Author: 豪
 */

#ifndef INC_KEYPAD_H_
#define INC_KEYPAD_H_

#include "stm32f4xx_hal.h"

void Keypad_Init(void);
char Keypad_GetKey(void);

#endif /* INC_KEYPAD_H_ */
