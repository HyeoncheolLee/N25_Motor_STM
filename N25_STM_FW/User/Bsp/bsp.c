/**
 * @file    bsp.c
 */
#include "bsp.h"
#include "board_config.h"
#include "main.h"
#include "tim.h"            /* htim6 */
#if (FEATURE_IWDG_ENABLE == 1)
#include "iwdg.h"           /* hiwdg */
#endif

static BSP_ResetCause_e s_reset_cause = BSP_RESET_OTHER;

static void BSP_ReadResetCause(void)
{
    if      (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) { s_reset_cause = BSP_RESET_IWDG; }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST)) { s_reset_cause = BSP_RESET_WWDG; }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))  { s_reset_cause = BSP_RESET_SOFTWARE; }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST))  { s_reset_cause = BSP_RESET_BROWNOUT; }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))  { s_reset_cause = BSP_RESET_PIN; }
    else                                           { s_reset_cause = BSP_RESET_POWER_ON; }
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

/*
 * 초기화 순서 (중요):
 *  1. 게이트드라이버 Disable, PWM 출력(MOE) OFF  ← 무엇보다 먼저
 *  2. 통신/저장장치 (SPI, CAN)
 *  3. 센싱 (ADC, Encoder)
 *  4. PWM 타이머 시작 (MOE=0 유지 → ADC 트리거만 동작)
 */
void BSP_Init(void)
{
    BSP_ReadResetCause();

    BSP_GPIO_Init();            /* DRV_EN=Low, WP=보호 */
    BSP_PWM_OutputDisable();

    BSP_SPI_Init();
    BSP_Brake_Init();           /* 브레이크 체결 상태 */
    BSP_ENC_Init();
    BSP_ADC_Init();
    BSP_PWM_Init();             /* 타이머 카운트 시작 → TRGO2 → ADC 20kHz 시작 */
}

BSP_ResetCause_e BSP_GetResetCause(void)
{
    return s_reset_cause;
}

void BSP_WatchdogKick(void)
{
#if (FEATURE_IWDG_ENABLE == 1)
    HAL_IWDG_Refresh(&hiwdg);
#endif
}

uint32_t BSP_GetTickMs(void)
{
    return HAL_GetTick();
}

void BSP_DelayMs(uint32_t ms)
{
    HAL_Delay(ms);
}

void BSP_SystemReset(void)
{
    BSP_PWM_OutputDisable();
    BSP_DRV_Disable();
    NVIC_SystemReset();
}

uint32_t BSP_GetUid32(void)
{
    return HAL_GetUIDw0() ^ HAL_GetUIDw1() ^ HAL_GetUIDw2();
}

void BSP_StartSchedulerTimer(void)
{
    HAL_TIM_Base_Start_IT(&htim6);
}

__weak void BSP_OnTick1ms(void)
{
}

/*
 * HAL 공용 콜백. CubeMX 에서 HAL Timebase 를 SysTick 으로 두어야
 * main.c 에 이 함수가 생성되지 않는다 (중복 정의 방지).
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        BSP_OnTick1ms();
    }
}
