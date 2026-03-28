/*
 * rc522.h
 *
 *  Created on: Mar 26, 2026
 *      Author: 豪
 */

#ifndef __RC522_H
#define __RC522_H

#include "main.h"
#include <stdint.h>

#define RC522_CS_PORT       GPIOA
#define RC522_CS_PIN        GPIO_PIN_4

#define RC522_RST_PORT      GPIOB
#define RC522_RST_PIN       GPIO_PIN_0

extern SPI_HandleTypeDef hspi1;

/* MFRC522 Registers */
#define CommandReg          0x01
#define ComIEnReg           0x02
#define DivIEnReg           0x03
#define ComIrqReg           0x04
#define DivIrqReg           0x05
#define ErrorReg            0x06
#define Status1Reg          0x07
#define Status2Reg          0x08
#define FIFODataReg         0x09
#define FIFOLevelReg        0x0A
#define WaterLevelReg       0x0B
#define ControlReg          0x0C
#define BitFramingReg       0x0D
#define CollReg             0x0E
#define ModeReg             0x11
#define TxModeReg           0x12
#define RxModeReg           0x13
#define TxControlReg        0x14
#define TxASKReg            0x15
#define CRCResultRegM       0x21
#define CRCResultRegL       0x22
#define TModeReg            0x2A
#define TPrescalerReg       0x2B
#define TReloadRegH         0x2C
#define TReloadRegL         0x2D
#define VersionReg          0x37

/* RC522 command */
#define PCD_IDLE            0x00
#define PCD_AUTHENT         0x0E
#define PCD_RECEIVE         0x08
#define PCD_TRANSMIT        0x04
#define PCD_TRANSCEIVE      0x0C
#define PCD_RESETPHASE      0x0F
#define PCD_CALCCRC         0x03

/* PICC command */
#define PICC_REQIDL         0x26
#define PICC_REQALL         0x52
#define PICC_ANTICOLL       0x93

#define MI_OK               0
#define MI_NOTAGERR         1
#define MI_ERR              2

void RC522_Init(void);
uint8_t RC522_Request(uint8_t reqMode, uint8_t *tagType);
uint8_t RC522_Anticoll(uint8_t *serNum);
uint8_t RC522_ReadRegister(uint8_t reg);

#endif
