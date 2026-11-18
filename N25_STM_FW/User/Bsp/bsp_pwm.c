/**
 * @file    bsp_pwm.c
 */
#include "bsp_pwm.h"
#include "board_config.h"
#include "tim.h"            /* CubeMX: extern TIM_HandleTypeDef htim1; */

static volatile bool     s_break_latched = false;
static volatile uint32_t s_break_cnt     = 0U;

static inline uint16_t PWM_Clamp(uint16_t cmp)
{
    const uint16_t max_cmp = (uint16_t)((uint32_t)PWM_ARR * PWM_DUTY_MAX_PERMIL / 1000U);
    return (cmp > max_cmp) ? max_cmp : cmp;
}

void BSP_PWM_Init(void)
{
    /* 1) 출력 차단 상태에서 시작 */
    __HAL_TIM_MOE_DISABLE_UNCONDITIONALLY(&htim1);

    /* 2) 50% 듀티 (모든 상 동일 → 선간 전압 0) */
    BSP_PWM_SetCompare(PWM_ARR / 2U, PWM_ARR / 2U, PWM_ARR / 2U);

    /* 3) ADC 트리거: CH4 (출력 없음). CCR4 = ARR - offset → 카운터 peak(low-side ON 중앙) */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, PWM_ARR - PWM_ADC_TRIG_OFFSET);

    /* 4) 채널 시작 (MOE=0 이므로 실제 핀 출력은 아직 idle(Low)) */
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);

    /* HAL_TIM_PWM_Start 는 MOE 를 켠다 → 즉시 다시 끈다.
     * 이 순간 DRV_EN = Low(BSP_GPIO_Init) 이므로 게이트는 구동되지 않는다.
     * → BSP_PWM_Init() 은 반드시 DRV8353 Enable 이전에 호출할 것 (BSP_Init 순서) */
    __HAL_TIM_MOE_DISABLE_UNCONDITIONALLY(&htim1);

    /* 5) Break 인터럽트 */
    __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_BREAK);
    __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_BREAK);
}

void BSP_PWM_OutputEnable(void)
{
    if (s_break_latched)
    {
        return;                         /* Fault 래치 중에는 출력 금지 */
    }
    __HAL_TIM_MOE_ENABLE(&htim1);
}

void BSP_PWM_OutputDisable(void)
{
    __HAL_TIM_MOE_DISABLE_UNCONDITIONALLY(&htim1);
}

bool BSP_PWM_IsOutputEnabled(void)
{
    return ((htim1.Instance->BDTR & TIM_BDTR_MOE) != 0U);
}

void BSP_PWM_SetCompare(uint16_t cmp_u, uint16_t cmp_v, uint16_t cmp_w)
{
    /* Preload 활성 → 다음 update 이벤트에 동시 반영 */
    htim1.Instance->CCR1 = PWM_Clamp(cmp_u);
    htim1.Instance->CCR2 = PWM_Clamp(cmp_v);
    htim1.Instance->CCR3 = PWM_Clamp(cmp_w);
}

void BSP_PWM_SetDutyPermil(uint16_t u, uint16_t v, uint16_t w)
{
    BSP_PWM_SetCompare((uint16_t)((uint32_t)PWM_ARR * u / 1000U),
                       (uint16_t)((uint32_t)PWM_ARR * v / 1000U),
                       (uint16_t)((uint32_t)PWM_ARR * w / 1000U));
}

uint16_t BSP_PWM_GetPeriod(void)
{
    return (uint16_t)PWM_ARR;
}

bool BSP_PWM_IsBreakActive(void)
{
    return s_break_latched;
}

void BSP_PWM_ClearBreak(void)
{
    /* nFAULT 가 아직 Low 면 BIF 가 다시 set 되므로 클리어 불가 */
    __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_BREAK);
    if (__HAL_TIM_GET_FLAG(&htim1, TIM_FLAG_BREAK) == RESET)
    {
        s_break_latched = false;
        __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_BREAK);
    }
}

uint32_t BSP_PWM_GetBreakCount(void)
{
    return s_break_cnt;
}

/* ------------------------------------------------------------------------ */
/* HAL callback: Break (nFAULT) — HW 가 이미 MOE=0 으로 출력 차단한 뒤 호출됨  */
/* ------------------------------------------------------------------------ */
__weak void BSP_PWM_OnBreak(void)
{
    /* svc_fault 에서 재정의 (strong) */
}

void HAL_TIMEx_BreakCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        /* nFAULT 가 Low 로 유지되면 BIF 가 계속 재설정 → 인터럽트 폭주 방지 */
        __HAL_TIM_DISABLE_IT(htim, TIM_IT_BREAK);
        s_break_latched = true;
        s_break_cnt++;
        BSP_PWM_OnBreak();
    }
}
