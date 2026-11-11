#include "parqueo_neopixel.h"

extern TIM_HandleTypeDef htim1;
HAL_StatusTypeDef Neo_UltimoEstado = HAL_OK;
#define PREFIJO 8U
#define DATOS (4U * 24U)
#define RESET 320U /* 400 us bajos a 800 kHz. */
static uint32_t pwm[PREFIJO + DATOS + RESET];
static uint32_t cero, uno, inicio;
static volatile uint8_t ocupado, forzar;
static uint8_t anterior, validosAnteriores, listo;

HAL_StatusTypeDef Neo_Iniciar(uint32_t reloj)
{
    uint32_t ticks = reloj / 800000UL;
    if (reloj % 800000UL || ticks < 8 || ticks > 65535) return HAL_ERROR;
    ocupado = listo = 0;
    forzar = 1;
    __HAL_TIM_SET_PRESCALER(&htim1, 0);
    __HAL_TIM_SET_AUTORELOAD(&htim1, ticks - 1U);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    /* Aproximadamente 0.35 us para cero y 0.70 us para uno. */
    cero = (reloj / 1000000UL * 35UL + 50UL) / 100UL;
    uno = (reloj / 1000000UL * 70UL + 50UL) / 100UL;
    Neo_UltimoEstado = HAL_OK;
    listo = 1;
    return HAL_OK;
}

void Neo_Tarea(uint8_t estado, uint8_t validos)
{
    if (!listo) return;
    if (ocupado) {
        if ((uint32_t)(HAL_GetTick() - inicio) > 20U) {
            HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
            ocupado = 0;
            forzar = 1;
            Neo_UltimoEstado = HAL_TIMEOUT;
        }
        return;
    }
    estado &= 0x0F;
    validos &= 0x0F;
    if (!forzar && estado == anterior && validos == validosAnteriores) return;
    unsigned p = 0;
    for (unsigned i = 0; i < PREFIJO; i++) pwm[p++] = 0;
    for (unsigned i = 0; i < 4; i++) {
        uint8_t r = 0, g = 0;
        if (!(validos & (1U << i))) { r = 8; g = 4; } /* Ambar: sin datos */
        else if (estado & (1U << i)) r = 16;
        else g = 16;
        uint8_t grb[] = {g, r, 0}; /* Brillo bajo para las primeras pruebas. */
        for (unsigned c = 0; c < 3; c++) {
            for (int bit = 7; bit >= 0; bit--) {
                pwm[p++] = (grb[c] & (1U << bit)) ? uno : cero;
            }
        }
    }
    for (unsigned i = 0; i < RESET; i++) pwm[p++] = 0;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    inicio = HAL_GetTick();
    forzar = 0;
    ocupado = 1;
    Neo_UltimoEstado = HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, pwm, p);
    if (Neo_UltimoEstado != HAL_OK) { ocupado = 0; forzar = 1; }
    else { anterior = estado; validosAnteriores = validos; }
}

void Neo_DMAFin(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != htim1.Instance) return;
    /* El DMA NORMAL ya termino y su handle esta READY.
     * Stop_DMA intentaria abortarlo y puede generar NO_XFER dentro de la
     * misma IRQ, seguido de una callback de error y reintentos continuos. */
    __HAL_TIM_DISABLE_DMA(htim, TIM_DMA_CC1);
    __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, 0);
    HAL_TIM_PWM_Stop(htim, TIM_CHANNEL_1);
    ocupado = 0;
}

void Neo_DMAError(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != htim1.Instance) return;
    HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1);
    Neo_UltimoEstado = HAL_ERROR;
    ocupado = 0;
    forzar = 1;
}
