#include "parqueo_display7.h"

static const uint16_t pines[7] = {
    GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7,
    GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10
};
/* Bits a,b,c,d,e,f,g: 1 significa segmento iluminado. */
static const uint8_t numeros[9] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F
};
static uint8_t anodo, previo = 0xFF;

static void escribir(uint8_t patron)
{
    if (patron == previo) return;
    for (uint8_t i = 0; i < 7; i++) {
        uint8_t encendido = (patron >> i) & 1U;
        GPIO_PinState nivel = (encendido != anodo)
            ? GPIO_PIN_SET : GPIO_PIN_RESET;
        HAL_GPIO_WritePin(GPIOC, pines[i], nivel);
    }
    previo = patron;
}

void Display7_Iniciar(uint8_t anodoComun)
{
    anodo = anodoComun ? 1U : 0U;
    previo = 0xFF;
    const uint16_t mascara = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 |
        GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
    __HAL_RCC_GPIOC_CLK_ENABLE();
    /* Cargar el nivel apagado antes de habilitar las salidas. */
    HAL_GPIO_WritePin(GPIOC, mascara, anodo ? GPIO_PIN_SET : GPIO_PIN_RESET);
    GPIO_InitTypeDef cfg = {0};
    cfg.Pin = mascara;
    cfg.Mode = GPIO_MODE_OUTPUT_PP;
    cfg.Pull = GPIO_NOPULL;
    cfg.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &cfg);
    Display7_MostrarGuion();
}

void Display7_MostrarNumero(uint8_t numero)
{
    escribir(numero <= 8 ? numeros[numero] : 0x40U);
}

void Display7_MostrarGuion(void) { escribir(0x40); }

void Display7_MostrarEstado(uint8_t estadoGlobal, uint8_t validosGlobal)
{
    if (validosGlobal != 0xFF) {
        Display7_MostrarGuion();
        return;
    }
    uint8_t libres = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (!(estadoGlobal & (1U << i))) libres++;
    }
    Display7_MostrarNumero(libres);
}
