#ifndef PARQUEO_ENLACE_H
#define PARQUEO_ENLACE_H
#include "stm32f4xx_hal.h"
#include <stdint.h>
/* Esclavo I2C: lectura de cuatro estados 0/1/2; escritura global de dos bytes. */
HAL_StatusTypeDef Enlace_Iniciar(I2C_HandleTypeDef *h,uint8_t direccion7);
void Enlace_Tarea(const volatile uint8_t estado[4]);
/* Devuelve ceros en validos si no hay una escritura global reciente. */
void Enlace_Global(uint8_t *ocupados,uint8_t *validos);
extern volatile uint32_t enlaceLecturas,enlaceRecepciones,enlaceErrores,enlaceErrorHAL;
#endif
