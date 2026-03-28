/*
 * auth.h
 *
 *  Created on: Mar 23, 2026
 *      Author: 豪
 */

#ifndef INC_AUTH_H_
#define INC_AUTH_H_

#include <stdbool.h>
#include <stdint.h>

#define MAX_PIN_LEN 8

void Auth_Init(void);
void Auth_Clear(void);
bool Auth_AddChar(char key);
bool Auth_IsClearKey(char key);
bool Auth_IsEnterKey(char key);
bool Auth_CheckPin(void);
const char* Auth_GetMaskedPin(void);
bool Auth_Backspace(void);

#endif /* INC_AUTH_H_ */
