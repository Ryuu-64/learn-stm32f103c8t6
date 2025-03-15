#include "stm32f1xx_hal.h"

/**
 * @brief  初始化 DWT 计数器
 * @note   启用跟踪功能，并将 DWT->CYCCNT 重置为 0，然后启用计数器
 */
void DWT_Init(void) {
    if ((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) == 0) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    }

    // 重置计数器
    DWT->CYCCNT = 0U;
    // 使能周期计数器
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t DWT_GetCycleCount(void) {
    return DWT->CYCCNT;
}

/**
  * @brief DWT计数实现精确延时，32位计数器
  */
void DWT_DelayUs(const uint32_t time_in_us) {
    const uint32_t second_to_us = 1000000;
    const uint32_t tick_duration = time_in_us * (SystemCoreClock / second_to_us);
    const uint32_t tick_start = DWT_GetCycleCount();
    while (DWT_GetCycleCount() - tick_start < tick_duration);
}
