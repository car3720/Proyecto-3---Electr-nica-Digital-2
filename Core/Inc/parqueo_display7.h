#ifndef PARQUEO_DISPLAY7_H
#define PARQUEO_DISPLAY7_H
#include "stm32f4xx_hal.h"
#include <stdint.h>

/* Un digito de siete segmentos SIN controlador incorporado.
 * a=PC4, b=PC5, c=PC6, d=PC7, e=PC8, f=PC9, g=PC10.
 * Una resistencia por segmento; identificar el comun y pines del modelo.
 * Iniciar despues de las MX_*_Init(), en Nucleo B. */
void Display7_Iniciar(uint8_t anodoComun); /* 0 catodo; 1 anodo */
void Display7_MostrarNumero(uint8_t numero); /* 0..8; otros -> guion */
void Display7_MostrarGuion(void);
/* Bits globales de OCHO espacios. Si falta alguno, muestra guion. */
void Display7_MostrarEstado(uint8_t estadoGlobal, uint8_t validosGlobal);
#endif
