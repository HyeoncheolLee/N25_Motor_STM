/**
 * @file    bsp_brake.c
 */
#include "bsp_brake.h"
#include "tim.h"            /* CubeMX: htim17 */

#define BRAKE_PULLIN_PERMIL     (500U)
#define BRAKE_HOLD_PERMIL       (230U)
#define BRAKE_PULLIN_MS         (150U)

typedef enum
{
    BRAKE_ST_ENGAGED = 0,
    BRAKE_ST_PULLIN,
    BRAKE_ST_HOLD
} Brake_State_e;

static Brake_State_e s_state = BRAKE_ST_ENGAGED;
static uint16_t      s_timer_ms;

static void Brake_SetPermil(uint16_t permil)
{
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim17);
    __HAL_TIM_SET_COMPARE(&htim17, TIM_CHANNEL_1, (arr + 1U) * permil / 1000U);
}

void BSP_Brake_Init(void)
{
    Brake_SetPermil(0U);
    HAL_TIM_PWM_Start(&htim17, TIM_CHANNEL_1);  /* TIM17 은 BDTR 있음 → HAL 이 MOE 설정 */
    s_state = BRAKE_ST_ENGAGED;
}

void BSP_Brake_Release(void)
{
    if (s_state == BRAKE_ST_ENGAGED)
    {
        Brake_SetPermil(BRAKE_PULLIN_PERMIL);
        s_timer_ms = BRAKE_PULLIN_MS;
        s_state    = BRAKE_ST_PULLIN;
    }
}

void BSP_Brake_Engage(void)
{
    Brake_SetPermil(0U);
    s_state = BRAKE_ST_ENGAGED;
}

bool BSP_Brake_IsReleased(void)
{
    return (s_state == BRAKE_ST_HOLD);
}

void BSP_Brake_Task1ms(void)
{
    if (s_state == BRAKE_ST_PULLIN)
    {
        if (s_timer_ms > 0U)
        {
            s_timer_ms--;
        }
        else
        {
            Brake_SetPermil(BRAKE_HOLD_PERMIL);
            s_state = BRAKE_ST_HOLD;
        }
    }
}
