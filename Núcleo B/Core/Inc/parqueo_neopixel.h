#ifndef PARQUEO_NEOPIXEL_H
#define PARQUEO_NEOPIXEL_H
#include "stm32f4xx_hal.h"
#include <stdint.h>
/* Cuatro WS2812B RGB GRB, 800 kHz. TIM1_CH1/PA8, DMA de PALABRA a PALABRA.
 * No sirve sin adaptar a modelos RGBW o de otro protocolo/orden de color. */
extern HAL_StatusTypeDef Neo_UltimoEstado;
HAL_StatusTypeDef Neo_Iniciar(uint32_t relojTim1);
void Neo_Tarea(uint8_t ocupacion, uint8_t validos);
void Neo_DMAFin(TIM_HandleTypeDef *htim);
void Neo_DMAError(TIM_HandleTypeDef *htim);
#endif
