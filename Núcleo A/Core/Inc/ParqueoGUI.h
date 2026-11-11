#ifndef PARQUEO_GUI_H_
#define PARQUEO_GUI_H_
#include <stdint.h>

/* 0=sin datos, 1=libre, 2=ocupado */
typedef enum { GUI_SIN_DATOS=0, GUI_LIBRE=1, GUI_OCUPADO=2 } ParqueoEstado_t;

void ParqueoGUI_Init(void);
void ParqueoGUI_SetEstado(uint8_t plaza_1_a_8, ParqueoEstado_t estado);
void ParqueoGUI_Update(void);

#endif
