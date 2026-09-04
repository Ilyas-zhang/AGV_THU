/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define RRGB_R_Pin GPIO_PIN_2
#define RRGB_R_GPIO_Port GPIOE
#define RRGB_G_Pin GPIO_PIN_3
#define RRGB_G_GPIO_Port GPIOE
#define RRGB_B_Pin GPIO_PIN_4
#define RRGB_B_GPIO_Port GPIOE
#define left_infrared_Pin GPIO_PIN_5
#define left_infrared_GPIO_Port GPIOE
#define right_infrared_Pin GPIO_PIN_6
#define right_infrared_GPIO_Port GPIOE
#define SonicTrig_Pin GPIO_PIN_11
#define SonicTrig_GPIO_Port GPIOF
#define SonicEcho_Pin GPIO_PIN_12
#define SonicEcho_GPIO_Port GPIOF
#define X1_Pin GPIO_PIN_13
#define X1_GPIO_Port GPIOF
#define X2_Pin GPIO_PIN_14
#define X2_GPIO_Port GPIOF
#define X3_Pin GPIO_PIN_15
#define X3_GPIO_Port GPIOF
#define X4_Pin GPIO_PIN_0
#define X4_GPIO_Port GPIOG
#define LRGB_G_Pin GPIO_PIN_7
#define LRGB_G_GPIO_Port GPIOE
#define Motor1_IN1_Pin GPIO_PIN_9
#define Motor1_IN1_GPIO_Port GPIOE
#define Motor1_IN2_Pin GPIO_PIN_11
#define Motor1_IN2_GPIO_Port GPIOE
#define Motor2_IN1_Pin GPIO_PIN_13
#define Motor2_IN1_GPIO_Port GPIOE
#define Motor2_IN2_Pin GPIO_PIN_14
#define Motor2_IN2_GPIO_Port GPIOE
#define LRGB_B_Pin GPIO_PIN_2
#define LRGB_B_GPIO_Port GPIOG
#define KEY1_Pin GPIO_PIN_3
#define KEY1_GPIO_Port GPIOG
#define KEY2_Pin GPIO_PIN_4
#define KEY2_GPIO_Port GPIOG
#define KEY3_Pin GPIO_PIN_5
#define KEY3_GPIO_Port GPIOG
#define Motor3_IN1_Pin GPIO_PIN_6
#define Motor3_IN1_GPIO_Port GPIOC
#define Motor3_IN2_Pin GPIO_PIN_7
#define Motor3_IN2_GPIO_Port GPIOC
#define Motor4_IN1_Pin GPIO_PIN_8
#define Motor4_IN1_GPIO_Port GPIOC
#define Motor4_IN2_Pin GPIO_PIN_9
#define Motor4_IN2_GPIO_Port GPIOC
#define Buzzer_Pin GPIO_PIN_12
#define Buzzer_GPIO_Port GPIOG

/* USER CODE BEGIN Private defines */
/* CubeMX 重生成后丢失的用户标签，需手动补回 */
#define LRGB_R_Pin         GPIO_PIN_1
#define LRGB_R_GPIO_Port   GPIOG
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
