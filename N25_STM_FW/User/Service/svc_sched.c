/**
 * @file    svc_sched.c
 */
#include "svc_sched.h"
#include "bsp.h"
#include <stddef.h>

static const uint16_t s_period_ms[SCHED_TASK_NUM] = { 1U, 10U, 100U, 1000U };

static Sched_TaskFunc_t  s_func[SCHED_TASK_NUM];
static volatile uint8_t  s_pending[SCHED_TASK_NUM];
static volatile uint32_t s_tick       = 0U;
static volatile uint32_t s_overrun    = 0U;

void SVC_Sched_Init(void)
{
    for (uint32_t i = 0U; i < SCHED_TASK_NUM; i++)
    {
        s_func[i]    = NULL;
        s_pending[i] = 0U;
    }
}

void SVC_Sched_Register(Sched_Task_e task, Sched_TaskFunc_t func)
{
    if (task < SCHED_TASK_NUM)
    {
        s_func[task] = func;
    }
}

/* TIM6 ISR (BSP hook 재정의) */
void BSP_OnTick1ms(void)
{
    s_tick++;
    for (uint32_t i = 0U; i < SCHED_TASK_NUM; i++)
    {
        if ((s_tick % s_period_ms[i]) == 0U)
        {
            if ((i == SCHED_TASK_1MS) && (s_pending[i] != 0U))
            {
                s_overrun++;        /* 이전 1ms 태스크가 아직 실행 안 됨 */
            }
            s_pending[i] = 1U;
        }
    }
}

void SVC_Sched_Run(void)
{
    /* 짧은 주기 우선 */
    for (uint32_t i = 0U; i < SCHED_TASK_NUM; i++)
    {
        if (s_pending[i] != 0U)
        {
            s_pending[i] = 0U;
            if (s_func[i] != NULL)
            {
                s_func[i]();
            }
        }
    }
}

uint32_t SVC_Sched_GetTick(void)         { return s_tick; }
uint32_t SVC_Sched_GetOverrunCount(void) { return s_overrun; }
