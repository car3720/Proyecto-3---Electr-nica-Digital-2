/* NUCLEO B P5-P8. Misma mediana y confirmaciones del ZIP nuevo de A.
 * Ocupado: 20..30 mm; otra distancia valida: libre; perdida: 8000 ms.
 * TRIG PB12..PB15, ECHO PC0..PC3. I2C 0x13 PB8/PB9.
 * NeoPixel PA8 TIM1_CH1. Display anodo comun PC4..PC10. */
#include "main.h"
#include "parqueo_enlace.h"
#include "parqueo_neopixel.h"
#include "parqueo_display7.h"
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch1;
volatile uint32_t neoFinalizadas,neoErrores,arranqueEtapa;
#define DISPLAY_ANODO_COMUN 1
#include <stdint.h>

#define PARQUEOS             4U
#define EST_LIBRE            0U
#define EST_OCUPADO          1U
#define EST_SIN_DATOS        2U
#define MIN_OCUPADO_MM      20U
#define MAX_OCUPADO_MM      30U
#define MIN_SENSOR_MM       20U
#define MAX_SENSOR_MM     4000U
#define MUESTRAS_FILTRO      3U
#define CONFIRMACIONES       3U
#define PAUSA_MS            80U
#define PERDIDA_SENSOR_MS 8000UL
#define ECHO_REPOSO_US    5000U
#define ECHO_SUBIDA_US   30000U
#define ECHO_ANCHO_US    25000U
#define ANCHO_MIN_US        90U

/* Diagnosticos para Live Expressions */
#define MED_OK               0U
#define MED_ECHO_ALTO        1U
#define MED_SIN_SUBIDA       2U
#define MED_ANCHO_EXCESIVO   3U
#define MED_PULSO_CORTO      4U
#define MED_FUERA_RANGO      5U

typedef struct {
    GPIO_TypeDef *trig_port;
    uint16_t trig_pin;
    GPIO_TypeDef *echo_port;
    uint16_t echo_pin;
} Sensor_t;

static const Sensor_t sensores[PARQUEOS] = {
    {GPIOB, GPIO_PIN_12, GPIOC, GPIO_PIN_0},
    {GPIOB, GPIO_PIN_13, GPIOC, GPIO_PIN_1},
    {GPIOB, GPIO_PIN_14, GPIOC, GPIO_PIN_2},
    {GPIOB, GPIO_PIN_15, GPIOC, GPIO_PIN_3}
};

volatile uint32_t diag_distancia_mm[4] = {0U};
volatile uint32_t diag_filtrada_mm[4] = {0U};
volatile uint32_t diag_ok[4] = {0U};
volatile uint32_t diag_fallos[4] = {0U};
volatile uint8_t diag_error[4] = {0U};
volatile uint8_t estado_sensor[4] = {2U,2U,2U,2U};
volatile uint32_t ultimo_ok_ms[4] = {0U};
volatile uint32_t diag_tim2_en_5ms = 0U;

static uint32_t historial[4][3] = {{0U}};
static uint8_t muestras[4] = {0U};
static uint8_t indice[4] = {0U};
static uint8_t candidato[4] = {2U,2U,2U,2U};
static uint8_t confirmaciones[4] = {0U};
static uint8_t alguna_valida[4] = {0U};

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void TIM2_1MHz_Init(void);
static void MX_TIM1_Init(void);
static void MX_DMA_Init(void);
static void actualizar_indicadores(void) {
    uint8_t eLocal=0,vLocal=0,e,v;
    for(uint8_t i=0;i<4;i++){
        if(estado_sensor[i]!=EST_SIN_DATOS)vLocal|=(uint8_t)(1U<<i);
        if(estado_sensor[i]==EST_OCUPADO)eLocal|=(uint8_t)(1U<<i);
    }
    Neo_Tarea(eLocal,vLocal);
    Enlace_Global(&e,&v);
    e=(e&0x0FU)|(uint8_t)(eLocal<<4);
    v=(v&0x0FU)|(uint8_t)(vLocal<<4);
    Display7_MostrarEstado(e,v);
}

/* TIM2: acceso directo a los registros; no necesita el modulo HAL_TIM. */
static uint32_t micros(void) {
    return TIM2->CNT;
}
static void espera_us(uint32_t us) {
    uint32_t t0 = micros();
    while ((uint32_t)(micros() - t0) < us) { }
}
static uint32_t mediana3(uint32_t a, uint32_t b, uint32_t c) {
    uint32_t t;
    if (a>b) {t=a;a=b;b=t;}
    if (b>c) {t=b;b=c;c=t;}
    if (a>b) {t=a;a=b;b=t;}
    return b;
}
static void actualizar_parqueo(uint8_t p,uint8_t nuevo) {
    if(estado_sensor[p]!=nuevo)estado_sensor[p]=nuevo;
}

/* Devuelve MED_OK y mm, o codigo de diagnostico; nunca bloquea indefinidamente. */
static uint8_t medir(uint8_t p, uint32_t *mm) {
    const Sensor_t *s = &sensores[p];
    uint32_t t0 = micros();
    /* Esperar retorno a reposo */
    while (HAL_GPIO_ReadPin(s->echo_port,s->echo_pin) == GPIO_PIN_SET) {
        if ((uint32_t)(micros()-t0) >= ECHO_REPOSO_US) return MED_ECHO_ALTO;
    }
    HAL_GPIO_WritePin(s->trig_port,s->trig_pin,GPIO_PIN_RESET);
    espera_us(3U);
    HAL_GPIO_WritePin(s->trig_port,s->trig_pin,GPIO_PIN_SET);
    espera_us(12U);
    HAL_GPIO_WritePin(s->trig_port,s->trig_pin,GPIO_PIN_RESET);
    t0 = micros();
    while (HAL_GPIO_ReadPin(s->echo_port,s->echo_pin) == GPIO_PIN_RESET) {
        if ((uint32_t)(micros()-t0) >= ECHO_SUBIDA_US) return MED_SIN_SUBIDA;
    }
    uint32_t subida = micros();
    while (HAL_GPIO_ReadPin(s->echo_port,s->echo_pin) == GPIO_PIN_SET) {
        if ((uint32_t)(micros()-subida) >= ECHO_ANCHO_US) return MED_ANCHO_EXCESIVO;
    }
    uint32_t ancho = (uint32_t)(micros()-subida);
    if (ancho < ANCHO_MIN_US) return MED_PULSO_CORTO;
    /* Calculo en mm, redondeado al entero mas cercano. */
    uint32_t medida = (ancho*10U+29U)/58U;
    if (medida < MIN_SENSOR_MM || medida > MAX_SENSOR_MM) return MED_FUERA_RANGO;
    *mm = medida;
    return MED_OK;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    arranqueEtapa=1;
    MX_GPIO_Init();
    Display7_Iniciar(DISPLAY_ANODO_COMUN);
    Display7_MostrarNumero(8);HAL_Delay(700);Display7_MostrarGuion();
    arranqueEtapa=2;
    TIM2_1MHz_Init();
    uint32_t t = micros();
    HAL_Delay(5U);
    diag_tim2_en_5ms = (uint32_t)(micros()-t);
    if (diag_tim2_en_5ms < 3500U || diag_tim2_en_5ms > 7000U) Error_Handler();

    MX_DMA_Init();MX_TIM1_Init();
    if(Neo_Iniciar(84000000UL)!=HAL_OK)Error_Handler();
    arranqueEtapa=3;
    if(Enlace_Iniciar(&hi2c1,0x13U)!=HAL_OK)Error_Handler();
    actualizar_indicadores();
    arranqueEtapa=4;

    uint8_t siguiente=0U;
    while (1) {
        uint8_t p=siguiente;
        uint32_t mm=0U;
        uint8_t codigo=medir(p,&mm);
        uint32_t ahora=HAL_GetTick();
        diag_error[p]=codigo;
        if (codigo == MED_OK) {
            diag_ok[p]++;
            diag_distancia_mm[p]=mm;
            /* El historial caducado no influye en una nueva lectura. */
            if (alguna_valida[p] && (uint32_t)(ahora-ultimo_ok_ms[p])>=PERDIDA_SENSOR_MS) {
                muestras[p]=0U; indice[p]=0U;
                candidato[p]=EST_SIN_DATOS;confirmaciones[p]=0U;
            }
            alguna_valida[p]=1U;
            ultimo_ok_ms[p]=ahora;
            historial[p][indice[p]]=mm;
            indice[p]=(uint8_t)((indice[p]+1U)%MUESTRAS_FILTRO);
            if (muestras[p] < MUESTRAS_FILTRO) muestras[p]++;
            /* No decidir hasta disponer de tres mediciones validas. */
            if (muestras[p] == MUESTRAS_FILTRO) {
                uint32_t filtrada=mediana3(historial[p][0],historial[p][1],historial[p][2]);
                diag_filtrada_mm[p]=filtrada;
                uint8_t nuevo=(filtrada>=MIN_OCUPADO_MM && filtrada<=MAX_OCUPADO_MM)
                              ? EST_OCUPADO : EST_LIBRE;
                if (nuevo==estado_sensor[p]) {
                    candidato[p]=nuevo;confirmaciones[p]=0U;
                } else {
                    if (candidato[p]!=nuevo) {candidato[p]=nuevo;confirmaciones[p]=1U;}
                    else if (confirmaciones[p]<CONFIRMACIONES) confirmaciones[p]++;
                    if (confirmaciones[p]>=CONFIRMACIONES) {
                        actualizar_parqueo(p,nuevo);
                        confirmaciones[p]=0U;
                    }
                }
            }
        } else {
            diag_fallos[p]++;
            /* Conservar estado ante fallos aislados, sin falsos cambios. */
            if (alguna_valida[p] && (uint32_t)(ahora-ultimo_ok_ms[p]) >= PERDIDA_SENSOR_MS) {
                actualizar_parqueo(p,EST_SIN_DATOS);
                muestras[p]=0U;indice[p]=0U;
                candidato[p]=EST_SIN_DATOS;confirmaciones[p]=0U;
            }
        }
        Enlace_Tarea(estado_sensor);
        actualizar_indicadores();
        HAL_Delay(PAUSA_MS);
        siguiente=(uint8_t)((siguiente+1U)%PARQUEOS);
    }
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
    osc.OscillatorType=RCC_OSCILLATORTYPE_HSI;
    osc.HSIState=RCC_HSI_ON;
    osc.HSICalibrationValue=RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState=RCC_PLL_ON;
    osc.PLL.PLLSource=RCC_PLLSOURCE_HSI;
    osc.PLL.PLLM=16U;osc.PLL.PLLN=336U;
    osc.PLL.PLLP=RCC_PLLP_DIV4;
    osc.PLL.PLLQ=2U;
    if (HAL_RCC_OscConfig(&osc)!=HAL_OK) Error_Handler();
    clk.ClockType=RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|
                  RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource=RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider=RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider=RCC_HCLK_DIV2;
    clk.APB2CLKDivider=RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk,FLASH_LATENCY_2)!=HAL_OK) Error_Handler();
}

static void TIM2_1MHz_Init(void) {
    __HAL_RCC_TIM2_CLK_ENABLE();
    TIM2->CR1=0U;
    TIM2->PSC=83U;                 /* Timer APB1 = 84 MHz */
    TIM2->ARR=0xFFFFFFFFU;
    TIM2->EGR=TIM_EGR_UG;
    TIM2->SR=0U;
    TIM2->CNT=0U;
    TIM2->CR1=TIM_CR1_CEN;
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef gpio={0};
    __HAL_RCC_GPIOA_CLK_ENABLE();__HAL_RCC_GPIOB_CLK_ENABLE();__HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15,GPIO_PIN_RESET);
    gpio.Pin=GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
    gpio.Mode=GPIO_MODE_OUTPUT_PP;gpio.Pull=GPIO_NOPULL;gpio.Speed=GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB,&gpio);
    gpio.Pin=GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
    gpio.Mode=GPIO_MODE_INPUT;gpio.Pull=GPIO_NOPULL;HAL_GPIO_Init(GPIOC,&gpio);
}

void Error_Handler(void) {
    __disable_irq();
    while (1) { }
}
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file,uint32_t line) {
    (void)file;(void)line;
}
#endif

static void MX_DMA_Init(void)
{
    __HAL_RCC_DMA2_CLK_ENABLE();
    HAL_NVIC_SetPriority(DMA2_Stream1_IRQn,5,0);
    HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
}

static void MX_TIM1_Init(void)
{
    TIM_OC_InitTypeDef pwm={0};
    htim1.Instance=TIM1;
    htim1.Init.Prescaler=0;
    htim1.Init.CounterMode=TIM_COUNTERMODE_UP;
    htim1.Init.Period=104;
    htim1.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
    htim1.Init.AutoReloadPreload=TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim1)!=HAL_OK) Error_Handler();
    pwm.OCMode=TIM_OCMODE_PWM1;
    pwm.Pulse=0;
    pwm.OCPolarity=TIM_OCPOLARITY_HIGH;
    pwm.OCFastMode=TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim1,&pwm,TIM_CHANNEL_1)!=HAL_OK) Error_Handler();
    HAL_TIM_MspPostInit(&htim1);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *h) {
    if(h->Instance==TIM1){neoFinalizadas++;Neo_DMAFin(h);}
}
void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *h) {
    if(h->Instance==TIM1){neoErrores++;Neo_DMAError(h);}
}
