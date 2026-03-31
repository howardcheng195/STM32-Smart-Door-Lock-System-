/*
 * rc522.c
 *
 *  Created on: Mar 26, 2026
 *      Author: 豪
 */
#include "rc522.h"

static void RC522_Select(void)
{
    HAL_GPIO_WritePin(RC522_CS_PORT, RC522_CS_PIN, GPIO_PIN_RESET);
}

static void RC522_Unselect(void)
{
    HAL_GPIO_WritePin(RC522_CS_PORT, RC522_CS_PIN, GPIO_PIN_SET);
}

static void RC522_Reset(void)
{
    HAL_GPIO_WritePin(RC522_RST_PORT, RC522_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(RC522_RST_PORT, RC522_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(10);
}

static void RC522_WriteRegister(uint8_t reg, uint8_t value)
{
    uint8_t data[2];
    data[0] = (reg << 1) & 0x7E;
    data[1] = value;

    RC522_Select();
    HAL_SPI_Transmit(&hspi1, data, 2, HAL_MAX_DELAY);
    RC522_Unselect();
}

uint8_t RC522_ReadRegister(uint8_t reg)
{
    uint8_t addr;
    uint8_t value;

    addr = ((reg << 1) & 0x7E) | 0x80;

    RC522_Select();
    HAL_SPI_Transmit(&hspi1, &addr, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, &value, 1, HAL_MAX_DELAY);
    RC522_Unselect();

    return value;
}

static void RC522_SetBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp;
    tmp = RC522_ReadRegister(reg);
    RC522_WriteRegister(reg, tmp | mask);
}

static void RC522_ClearBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp;
    tmp = RC522_ReadRegister(reg);
    RC522_WriteRegister(reg, tmp & (~mask));
}

static void RC522_AntennaOn(void)
{
    uint8_t temp = RC522_ReadRegister(TxControlReg);
    if ((temp & 0x03)!=0x03)
    {
        RC522_SetBitMask(TxControlReg, 0x03);
    }
}

static uint8_t RC522_ToCard(uint8_t command,
                            uint8_t *sendData,
                            uint8_t sendLen,
                            uint8_t *backData,
                            uint16_t *backLen)
{
    uint8_t status = MI_ERR;
    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;
    uint8_t lastBits;
    uint8_t n;
    uint16_t i;

    switch (command)
    {
        case PCD_AUTHENT:
            irqEn = 0x12;
            waitIRq = 0x10;
            break;
        case PCD_TRANSCEIVE:
            irqEn = 0x77;
            waitIRq = 0x30;
            break;
        default:
            break;
    }

    RC522_WriteRegister(ComIEnReg, irqEn | 0x80);
    RC522_ClearBitMask(ComIrqReg, 0x80);
    RC522_SetBitMask(FIFOLevelReg, 0x80);
    RC522_WriteRegister(CommandReg, PCD_IDLE);

    for (i = 0; i < sendLen; i++)
    {
        RC522_WriteRegister(FIFODataReg, sendData[i]);
    }

    RC522_WriteRegister(CommandReg, command);

    if (command == PCD_TRANSCEIVE)
    {
        RC522_SetBitMask(BitFramingReg, 0x80);
    }

    i = 2000;
    do
    {
        n = RC522_ReadRegister(ComIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

    RC522_ClearBitMask(BitFramingReg, 0x80);

    if (i != 0)
    {
        if ((RC522_ReadRegister(ErrorReg) & 0x1B) == 0x00)
        {
            status = MI_OK;

            if (n & irqEn & 0x01)
            {
                status = MI_NOTAGERR;
            }

            if (command == PCD_TRANSCEIVE)
            {
                n = RC522_ReadRegister(FIFOLevelReg);
                lastBits = RC522_ReadRegister(ControlReg) & 0x07;

                if (lastBits)
                    *backLen = (n - 1) * 8 + lastBits;
                else
                    *backLen = n * 8;

                if (n == 0) n = 1;
                if (n > 16) n = 16;

                for (i = 0; i < n; i++)
                {
                    backData[i] = RC522_ReadRegister(FIFODataReg);
                }
            }
        }
        else
        {
            status = MI_ERR;
        }
    }

    return status;
}

void RC522_Init(void)
{
    RC522_Unselect();
    RC522_Reset();

    RC522_WriteRegister(CommandReg, PCD_RESETPHASE);
    HAL_Delay(50);

    RC522_WriteRegister(TModeReg, 0x8D);
    RC522_WriteRegister(TPrescalerReg, 0x3E);
    RC522_WriteRegister(TReloadRegL, 30);
    RC522_WriteRegister(TReloadRegH, 0);

    RC522_WriteRegister(TxASKReg, 0x40);
    RC522_WriteRegister(ModeReg, 0x3D);

    RC522_AntennaOn();
}

uint8_t RC522_Request(uint8_t reqMode, uint8_t *tagType)
{
    uint8_t status;
    uint16_t backBits;

    RC522_WriteRegister(BitFramingReg, 0x07);

    tagType[0] = reqMode;
    status = RC522_ToCard(PCD_TRANSCEIVE, tagType, 1, tagType, &backBits);

    if ((status != MI_OK) || (backBits != 0x10))
    {
        status = MI_ERR;
    }

    return status;
}

uint8_t RC522_Anticoll(uint8_t *serNum)
{
    uint8_t status;
    uint8_t i;
    uint8_t serNumCheck = 0;
    uint16_t unLen;

    RC522_WriteRegister(BitFramingReg, 0x00);

    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;

    status = RC522_ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

    if (status == MI_OK)
    {
        for (i = 0; i < 4; i++)
        {
            serNumCheck ^= serNum[i];
        }

        if (serNumCheck != serNum[4])
        {
            status = MI_ERR;
        }
    }

    return status;
}



