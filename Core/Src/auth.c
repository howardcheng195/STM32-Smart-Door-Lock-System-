/*
 * auth.c
 *
 *  Created on: Mar 23, 2026
 *      Author: 豪
 */

#include "auth.h"
#include <string.h>

static char pin_buffer[MAX_PIN_LEN + 1];
static char masked_buffer[MAX_PIN_LEN + 1];
static uint8_t pin_len = 0;

static const char correct_pin[] = "1234";

void Auth_Init(void)
{
    Auth_Clear();
}

void Auth_Clear(void)
{
    memset(pin_buffer, 0, sizeof(pin_buffer));
    memset(masked_buffer, 0, sizeof(masked_buffer));
    pin_len = 0;
}

bool Auth_AddChar(char key)
{
    if (key < '0' || key > '9') {
        return false;
    }

    if (pin_len >= MAX_PIN_LEN) {
        return false;
    }

    pin_buffer[pin_len] = key;
    masked_buffer[pin_len] = '*';
    pin_len++;

    pin_buffer[pin_len] = '\0';
    masked_buffer[pin_len] = '\0';

    return true;
}

bool Auth_IsClearKey(char key)
{
    return (key == '*');
}

bool Auth_IsEnterKey(char key)
{
    return (key == '#');
}

bool Auth_CheckPin(void)
{
    return (strcmp(pin_buffer, correct_pin) == 0);
}

const char* Auth_GetMaskedPin(void)
{
    return masked_buffer;
}

bool Auth_Backspace(void)
{
    if (pin_len == 0)
    {
        return false;
    }

    pin_len--;

    pin_buffer[pin_len] = '\0';
    masked_buffer[pin_len] = '\0';

    return true;
}
