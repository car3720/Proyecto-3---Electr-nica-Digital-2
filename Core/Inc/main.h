#ifndef MAIN_H
#define MAIN_H
#include "stm32f4xx_hal.h"
void Error_Handler(void);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
#define SENSOR_P5_Pin GPIO_PIN_0
#define SENSOR_P6_Pin GPIO_PIN_1
#define SENSOR_P7_Pin GPIO_PIN_2
#define SENSOR_P8_Pin GPIO_PIN_3
#define SEG_A_Pin GPIO_PIN_4
#define SEG_B_Pin GPIO_PIN_5
#define SEG_C_Pin GPIO_PIN_6
#define SEG_D_Pin GPIO_PIN_7
#define SEG_E_Pin GPIO_PIN_8
#define SEG_F_Pin GPIO_PIN_9
#define SEG_G_Pin GPIO_PIN_10
#endif
