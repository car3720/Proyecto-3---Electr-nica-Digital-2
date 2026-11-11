#include "ParqueoGUI.h"
#include "ili9341.h"

/* TFT horizontal: 320 x 240. Colores RGB565. */
#define FONDO       0x0842U
#define CABECERA    0x10A3U
#define TARJETA     0x18E4U
#define TEXTO       0xFFFFU
#define VERDE       0x07E0U
#define ROJO        0xF800U
#define AMARILLO    0xFFE0U
#define GRIS        0x8410U
#define BORDE       0x52AAU

#define TARJ_W 75U
#define TARJ_H 60U

static ParqueoEstado_t estados[8];
static uint8_t previos[8];
static uint8_t iniciar_dibujo = 1U;
static uint8_t libres_prev = 255U;
static uint8_t ocupados_prev = 255U;
static uint8_t sin_datos_prev = 255U;

static void DibujarPlaza(uint8_t i)
{
    const uint16_t x = (uint16_t)(3U + (i % 4U) * 80U);
    const uint16_t y = (uint16_t)(i < 4U ? 57U : 139U);
    const ParqueoEstado_t estado = estados[i];
    const uint16_t color = (estado == GUI_LIBRE) ? VERDE :
                           (estado == GUI_OCUPADO ? ROJO : GRIS);
    const char *leyenda = (estado == GUI_LIBRE) ? "LIBRE" :
                          (estado == GUI_OCUPADO ? "OCUPADO" : "SIN DATOS");
    char titulo[3] = { 'P', (char)('1' + i), '\0' };
    uint16_t ancho_texto = (uint16_t)((estado == GUI_LIBRE ? 5U :
                             estado == GUI_OCUPADO ? 7U : 9U) * 8U);

    FillRect(x, y, TARJ_W, TARJ_H, TARJETA);
    Rect(x, y, TARJ_W - 1U, TARJ_H - 1U, BORDE);
    LCD_Print(titulo, x + 29, y + 5, 1, TEXTO, TARJETA);
    FillRect(x + 8U, y + 24U, TARJ_W - 16U, 9U, color);
    LCD_Print((char *)leyenda, (int)(x + (TARJ_W - ancho_texto)/2U),
              y + 40U, 1, TEXTO, TARJETA);
}

static void DibujarResumen(uint8_t libres, uint8_t ocupados, uint8_t sin_datos)
{
    char l[] = "LIBRES: 0";
    char o[] = "OCUP: 0";
    char s[] = "S/D: 0";
    l[8] = (char)('0' + libres);
    o[6] = (char)('0' + ocupados);
    s[5] = (char)('0' + sin_datos);

    FillRect(0U, 211U, 320U, 29U, CABECERA);
    LCD_Print(l, 8, 218, 1, VERDE, CABECERA);
    LCD_Print(o, 122, 218, 1, ROJO, CABECERA);
    LCD_Print(s, 239, 218, 1, AMARILLO, CABECERA);
}

void ParqueoGUI_Init(void)
{
    for (uint8_t i = 0U; i < 8U; i++)
    {
        estados[i] = GUI_SIN_DATOS;
        previos[i] = 255U;
    }
    libres_prev = ocupados_prev = sin_datos_prev = 255U;
    iniciar_dibujo = 1U;

    LCD_Init();
    LCD_Clear(FONDO);
    FillRect(0U, 0U, 320U, 32U, CABECERA);
    LCD_Print("CONTROL PARQUEOS", 32, 6, 2, TEXTO, CABECERA);
    LCD_Print("NUCLEO A - P1 A P4", 7, 39, 1, AMARILLO, FONDO);
    LCD_Print("NUCLEO B - P5 A P8", 7, 121, 1, AMARILLO, FONDO);
}

void ParqueoGUI_SetEstado(uint8_t plaza_1_a_8, ParqueoEstado_t estado)
{
    if (plaza_1_a_8 < 1U || plaza_1_a_8 > 8U)
        return;
    if (estado > GUI_OCUPADO)
        return;
    estados[plaza_1_a_8 - 1U] = estado;
}

void ParqueoGUI_Update(void)
{
    uint8_t libres = 0U, ocupados = 0U, sin_datos = 0U;
    for (uint8_t i = 0; i < 8U; i++)
    {
        if (estados[i] == GUI_LIBRE) ++libres;
        else if (estados[i] == GUI_OCUPADO) ++ocupados;
        else ++sin_datos;

        if (iniciar_dibujo || previos[i] != (uint8_t)estados[i])
        {
            DibujarPlaza(i);
            previos[i] = (uint8_t)estados[i];
        }
    }
    if (iniciar_dibujo || libres != libres_prev ||
        ocupados != ocupados_prev || sin_datos != sin_datos_prev)
    {
        DibujarResumen(libres, ocupados, sin_datos);
        libres_prev = libres;
        ocupados_prev = ocupados;
        sin_datos_prev = sin_datos;
    }
    iniciar_dibujo = 0U;
}
