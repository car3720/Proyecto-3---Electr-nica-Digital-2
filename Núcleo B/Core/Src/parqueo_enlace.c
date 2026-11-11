#include "parqueo_enlace.h"
static I2C_HandleTypeDef *bus;
static volatile uint32_t publicado=0x02020202U;
static volatile uint16_t globalRecibido;
static volatile uint32_t recibidoMs;
static volatile uint8_t hayGlobal,recuperar;
static uint8_t tx[4],rx[2];
volatile uint32_t enlaceLecturas,enlaceRecepciones,enlaceErrores,enlaceErrorHAL;

HAL_StatusTypeDef Enlace_Iniciar(I2C_HandleTypeDef *h,uint8_t direccion7)
{
    if(!h||direccion7>127U)return HAL_ERROR;
    bus=h;publicado=0x02020202U;globalRecibido=0;hayGlobal=recuperar=0;
    enlaceLecturas=enlaceRecepciones=enlaceErrores=enlaceErrorHAL=0;
    h->Instance=I2C1;h->Init.ClockSpeed=100000;
    h->Init.DutyCycle=I2C_DUTYCYCLE_2;h->Init.OwnAddress1=(uint32_t)direccion7<<1;
    h->Init.AddressingMode=I2C_ADDRESSINGMODE_7BIT;h->Init.DualAddressMode=I2C_DUALADDRESS_DISABLE;
    h->Init.OwnAddress2=0;h->Init.GeneralCallMode=I2C_GENERALCALL_DISABLE;
    h->Init.NoStretchMode=I2C_NOSTRETCH_DISABLE;
    if(HAL_I2C_Init(h)!=HAL_OK)return HAL_ERROR;
    return HAL_I2C_EnableListen_IT(h);
}
void Enlace_Tarea(const volatile uint8_t estado[4])
{
    if(!bus)return;
    uint32_t copia=0;
    for(uint8_t i=0;i<4;i++){
        uint8_t s=estado[i];if(s>2U)s=2;
        copia|=(uint32_t)s<<(i*8U);
    }
    publicado=copia; /* Publicacion atomica de los cuatro bytes en Cortex-M4. */
    if(recuperar){
        recuperar=0;
        if(HAL_I2C_DeInit(bus)!=HAL_OK||HAL_I2C_Init(bus)!=HAL_OK)recuperar=1;
    }
    if(!recuperar&&HAL_I2C_GetState(bus)==HAL_I2C_STATE_READY)
        if(HAL_I2C_EnableListen_IT(bus)!=HAL_OK)recuperar=1;
}
void Enlace_Global(uint8_t *ocupados,uint8_t *validos)
{
    uint32_t previo=__get_PRIMASK();__disable_irq();
    uint16_t dato=globalRecibido;uint32_t t=recibidoMs;uint8_t hay=hayGlobal;
    __set_PRIMASK(previo);
    *ocupados=(uint8_t)dato;
    *validos=(hay&&(uint32_t)(HAL_GetTick()-t)<2000U)?(uint8_t)(dato>>8):0;
}
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *h,uint8_t direccion,uint16_t coincidencia)
{
    (void)coincidencia;if(!bus||h->Instance!=bus->Instance)return;
    HAL_StatusTypeDef r;
    if(direccion==I2C_DIRECTION_RECEIVE){
        uint32_t copia=publicado;for(uint8_t i=0;i<4;i++)tx[i]=(uint8_t)(copia>>(i*8U));
        enlaceLecturas++;r=HAL_I2C_Slave_Seq_Transmit_IT(h,tx,4,I2C_FIRST_AND_LAST_FRAME);
    }else r=HAL_I2C_Slave_Seq_Receive_IT(h,rx,2,I2C_FIRST_AND_LAST_FRAME);
    if(r!=HAL_OK)recuperar=1;
}
void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *h)
{
    if(!bus||h->Instance!=bus->Instance)return;
    globalRecibido=(uint16_t)(rx[0]|((uint16_t)rx[1]<<8));
    recibidoMs=HAL_GetTick();hayGlobal=1;enlaceRecepciones++;
}
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *h)
{
    if(bus&&h->Instance==bus->Instance)
        if(HAL_I2C_EnableListen_IT(h)!=HAL_OK)recuperar=1;
}
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *h)
{
    if(!bus||h->Instance!=bus->Instance)return;
    enlaceErrorHAL=HAL_I2C_GetError(h);enlaceErrores++;
    if(enlaceErrorHAL&~HAL_I2C_ERROR_AF)recuperar=1;
}
