#include "main.h"
extern DMA_HandleTypeDef hdma_tim1_ch1;
void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE(); __HAL_RCC_PWR_CLK_ENABLE();
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
}
void HAL_I2C_MspInit(I2C_HandleTypeDef *h)
{
    if (h->Instance!=I2C1) return;
    __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_I2C1_CLK_ENABLE();
    GPIO_InitTypeDef g={0};
    g.Pin=GPIO_PIN_8|GPIO_PIN_9;g.Mode=GPIO_MODE_AF_OD;
    g.Pull=GPIO_NOPULL;g.Speed=GPIO_SPEED_FREQ_VERY_HIGH;g.Alternate=GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB,&g);
    HAL_NVIC_SetPriority(I2C1_EV_IRQn,6,0);HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn,6,0);HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
}
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *h)
{
    if (h->Instance!=I2C1) return;
    __HAL_RCC_I2C1_CLK_DISABLE();HAL_GPIO_DeInit(GPIOB,GPIO_PIN_8|GPIO_PIN_9);
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
}
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *h)
{
    if (h->Instance==TIM2) __HAL_RCC_TIM2_CLK_ENABLE();
    else if (h->Instance==TIM1) HAL_TIM_PWM_MspInit(h);
}
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *h)
{
    if (h->Instance!=TIM1) return;
    __HAL_RCC_TIM1_CLK_ENABLE(); __HAL_RCC_DMA2_CLK_ENABLE();
    hdma_tim1_ch1.Instance=DMA2_Stream1;
    hdma_tim1_ch1.Init.Channel=DMA_CHANNEL_6;
    hdma_tim1_ch1.Init.Direction=DMA_MEMORY_TO_PERIPH;
    hdma_tim1_ch1.Init.PeriphInc=DMA_PINC_DISABLE;
    hdma_tim1_ch1.Init.MemInc=DMA_MINC_ENABLE;
    hdma_tim1_ch1.Init.PeriphDataAlignment=DMA_PDATAALIGN_WORD;
    hdma_tim1_ch1.Init.MemDataAlignment=DMA_MDATAALIGN_WORD;
    hdma_tim1_ch1.Init.Mode=DMA_NORMAL;
    hdma_tim1_ch1.Init.Priority=DMA_PRIORITY_HIGH;
    hdma_tim1_ch1.Init.FIFOMode=DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_tim1_ch1)!=HAL_OK) Error_Handler();
    __HAL_LINKDMA(h,hdma[TIM_DMA_ID_CC1],hdma_tim1_ch1);
}
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *h)
{
    if (h->Instance!=TIM1) return;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g={0};
    g.Pin=GPIO_PIN_8;g.Mode=GPIO_MODE_AF_PP;g.Pull=GPIO_NOPULL;
    g.Speed=GPIO_SPEED_FREQ_VERY_HIGH;g.Alternate=GPIO_AF1_TIM1;
    HAL_GPIO_Init(GPIOA,&g);
}
