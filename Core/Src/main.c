/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "rc522.h"
#include "lcd_i2c.h"
#include "keypad.h"
#include "auth.h"
#include <stdio.h>
#include <string.h>
#include "bluetooth.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;
//SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
//static void MX_SPI2_Init(void);
static void MX_USART2_UART_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
#define CARD_UID_LEN 4
#define UNLOCK_DURATION_MS   5000
#define DENIED_DURATION_MS   2000
#define LOCKOUT_DURATION_MS  30000
#define MAX_FAILED_COUNT     5

extern SPI_HandleTypeDef hspi1;
extern I2C_HandleTypeDef hi2c1;

typedef struct
{
    uint8_t uid[CARD_UID_LEN];
    char name[16];
    // 可擴充功能
} RFID_Card_t;

RFID_Card_t allowed_cards[] = {
		{{0x1A, 0x18, 0x0E, 0x01}, "Hao"}
};

#define ALLOWED_CARD_COUNT (sizeof(allowed_cards) / sizeof(allowed_cards[0]))

typedef enum
{
    SYS_STATE_IDLE = 0,
    SYS_STATE_UNLOCKED,
    SYS_STATE_DENIED,
    SYS_STATE_LOCKOUT
} SystemState_t;

SystemState_t system_state = SYS_STATE_IDLE;

uint32_t unlock_start_time = 0;   // 合法開門開始時間
uint32_t denied_start_time = 0;   // 非法卡顯示開始時間
uint32_t lockout_start_time = 0;  // 鎖定開始時間

uint8_t failed_count = 0;         // 連續錯誤次數

// 避免同一張 RFID 一直重複觸發
uint8_t last_uid[4] = {0};
uint8_t has_last_uid = 0;

bool UID_IsSame(uint8_t *uid1, uint8_t *uid2)
{
    for (int i = 0; i < CARD_UID_LEN; i++)
    {
        if (uid1[i] != uid2[i])
        {
            return 0;
        }
    }
    return 1;
}

RFID_Card_t* FindAuthorizedCard(uint8_t *uid)
{
    for (int i = 0; i < ALLOWED_CARD_COUNT; i++)
    {
        if (UID_IsSame(uid, allowed_cards[i].uid))
        {
            return &allowed_cards[i];
        }
    }
    return NULL;
}

void Door_Lock(void)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
}

void Door_Unlock(void)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
}

//void LCD_ShowAccessGranted(uint8_t *uid)
//{
//    char line2[17];
//
//    snprintf(line2, sizeof(line2), "%02X %02X %02X %02X",
//             uid[0], uid[1], uid[2], uid[3]);
//
//    LCD_Clear();
//    LCD_PrintLine(0, "Access Granted");
//    LCD_PrintLine(1, line2);
//}
//
//void LCD_ShowAccessDenied(uint8_t *uid)
//{
//    char line2[17];
//
//    snprintf(line2, sizeof(line2), "%02X %02X %02X %02X",
//             uid[0], uid[1], uid[2], uid[3]);
//
//    LCD_Clear();
//    LCD_PrintLine(0, "Access Denied");
//    LCD_PrintLine(1, line2);
//}

void LCD_ShowIdle(void)
{
    LCD_Clear();
    LCD_PrintLine(0, "Scan Card or PIN");
    LCD_PrintLine(1, "Door Locked");
}

void LCD_ShowPINInput(void)
{
    char line2[17];

    snprintf(line2, sizeof(line2), "PIN: %s", Auth_GetMaskedPin());

    LCD_Clear();
    LCD_PrintLine(0, "Enter PIN");
    LCD_PrintLine(1, line2);
}

void LCD_ShowCardGranted(RFID_Card_t *card)
{
    char line2[17];

    snprintf(line2, sizeof(line2), "Hello %s", card->name);

    LCD_Clear();
    LCD_PrintLine(0, "Card Accepted");
    LCD_PrintLine(1, line2);
}

void LCD_ShowPINGranted(void)
{
    LCD_Clear();
    LCD_PrintLine(0, "PIN Correct");
    LCD_PrintLine(1, "Door Unlocked");
}

void LCD_ShowDenied(const char *msg1, uint8_t failed, uint8_t max_failed)
{
    char line2[17];

    snprintf(line2, sizeof(line2), "Fail: %d/%d", failed, max_failed);

    LCD_Clear();
    LCD_PrintLine(0, msg1);
    LCD_PrintLine(1, line2);
}

void LCD_ShowLockout(uint32_t remain_sec)
{
    char line2[17];

    snprintf(line2, sizeof(line2), "Remain: %lu s", remain_sec);

    LCD_Clear();
    LCD_PrintLine(0, "System Locked");
    LCD_PrintLine(1, line2);
}

void Enter_Idle_State(void)
{
    Door_Lock();
    Auth_Clear();
    LCD_ShowIdle();
//    BT_Send("EVENT:LOCKED\r\n");
    system_state = SYS_STATE_IDLE;
}

void Enter_Unlocked_State(void)
{
    failed_count = 0;
    Door_Unlock();
    BT_Send("EVENT:UNLOCKED\r\n");
    system_state = SYS_STATE_UNLOCKED;
    unlock_start_time = HAL_GetTick();
}

void Enter_Denied_State(const char *msg1)
{
    Door_Lock();
    LCD_ShowDenied(msg1, failed_count, MAX_FAILED_COUNT);
//    BT_Send("EVENT:WRONG_PIN\r\n");
    system_state = SYS_STATE_DENIED;
    denied_start_time = HAL_GetTick();
}

uint32_t last_lockout_remain = 0xFFFFFFFF;

void Enter_Lockout_State(void)
{
    Door_Lock();
    BT_Send("EVENT:LOCKOUT\r\n");
    system_state = SYS_STATE_LOCKOUT;
    lockout_start_time = HAL_GetTick();
    last_lockout_remain = 0xFFFFFFFF;
    LCD_ShowLockout(LOCKOUT_DURATION_MS / 1000);
}

void Process_RFID(uint8_t *tagType, uint8_t *uid)
{
    if (RC522_Request(PICC_REQIDL, tagType) == MI_OK)
    {
        if (RC522_Anticoll(uid) == MI_OK)
        {
            if (!has_last_uid || !UID_IsSame(uid, last_uid))
            {
                for (int i = 0; i < CARD_UID_LEN; i++)
                {
                    last_uid[i] = uid[i];
                }
                has_last_uid = 1;

                RFID_Card_t *card = FindAuthorizedCard(uid);
                // 在名單的 card
                if (card != NULL)
                {
                    char bt_msg[32];

                    LCD_ShowCardGranted(card);

                    snprintf(bt_msg, sizeof(bt_msg), "EVENT:ACCESS_GRANTED,USER:%s\r\n", card->name);
                    BT_Send(bt_msg);

                    Enter_Unlocked_State();
                }
                else
                {
                    failed_count++;

                    if (failed_count >= MAX_FAILED_COUNT)
                    {
                        Enter_Lockout_State();
                    }
                    else
                    {
                        Enter_Denied_State("Access Denied");
                        BT_Send("EVENT:ACCESS_DENIED\r\n");
                    }
                }
            }
        }
    }
    else
    {
        has_last_uid = 0;
    }
}

void Process_Keypad(void)
{
    char key = Keypad_GetKey();

    if (key == 0)
    {
        return;
    }

    if (Auth_IsClearKey(key))
    {
    	// 清掉全部
//        Auth_Clear();
    	if (Auth_Backspace()){
    		LCD_ShowPINInput();
    	}
    }
    else if (Auth_IsEnterKey(key))
    {
    	if (Auth_CheckPin())
    	{
    	    LCD_ShowPINGranted();
    	    BT_Send("EVENT:PIN_OK\r\n");
    	    Enter_Unlocked_State();
    	}
        else
        {
            failed_count++;

            if (failed_count >= MAX_FAILED_COUNT)
            {
                Enter_Lockout_State();
            }
            else
            {
                Enter_Denied_State("Wrong PIN");
                BT_Send("EVENT:WRONG_PIN\r\n");
            }
        }

        Auth_Clear();
    }
    else
    {
        if (Auth_AddChar(key))
        {
            LCD_ShowPINInput();
        }
    }
}

int main(void)
{
    uint8_t tagType[2];
    uint8_t uid[5];

    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_SPI1_Init();
    MX_USART2_UART_Init();

    LCD_Init(&hi2c1);
    Keypad_Init();
    Auth_Init();
    RC522_Init();
    BT_Init(&huart2);

    Enter_Idle_State();

    while (1)
    {
        uint32_t now = HAL_GetTick();

        if (system_state == SYS_STATE_IDLE)
        {
            Process_RFID(tagType, uid);
            Process_Keypad();
        }
        else if (system_state == SYS_STATE_UNLOCKED)
        {
            if (now - unlock_start_time >= UNLOCK_DURATION_MS)
            {
            	BT_Send("EVENT:LOCKED\r\n");
                Enter_Idle_State();
                has_last_uid = 0;
            }
        }
        else if (system_state == SYS_STATE_DENIED)
        {
            if (now - denied_start_time >= DENIED_DURATION_MS)
            {
                Enter_Idle_State();
                has_last_uid = 0;
            }
        }
        else if (system_state == SYS_STATE_LOCKOUT)
        {
            uint32_t elapsed = now - lockout_start_time;

            if (elapsed >= LOCKOUT_DURATION_MS)
            {
                failed_count = 0;
                last_lockout_remain = 0xFFFFFFFF;
                Enter_Idle_State();
                has_last_uid = 0;
            }
            else
            {
                uint32_t remain_sec = (LOCKOUT_DURATION_MS - elapsed) / 1000 + 1;

                if (remain_sec != last_lockout_remain)
                {
                    last_lockout_remain = remain_sec;
                    LCD_ShowLockout(remain_sec);
                }
            }
        }

        HAL_Delay(50);
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin
                          |Audio_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PE3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PC0 PC1 PC2 PC3 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PC4 PC5 PC6 PC7 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : BOOT1_Pin */
  GPIO_InitStruct.Pin = BOOT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CLK_IN_Pin */
  GPIO_InitStruct.Pin = CLK_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(CLK_IN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD4_Pin LD3_Pin LD5_Pin LD6_Pin
                           Audio_RST_Pin */
  GPIO_InitStruct.Pin = LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin
                          |Audio_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : I2S3_SCK_Pin I2S3_SD_Pin */
  GPIO_InitStruct.Pin = I2S3_SCK_Pin|I2S3_SD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MEMS_INT2_Pin */
  GPIO_InitStruct.Pin = MEMS_INT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MEMS_INT2_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM7 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM7) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
