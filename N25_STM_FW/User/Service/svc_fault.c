/**
 * @file    svc_fault.c
 */
#include "svc_fault.h"
#include "svc_param.h"
#include "board_config.h"
#include "bsp.h"
#include <string.h>

#define FAULT_CLEAR_WINDOW_MS   (60000U)

static Fault_Status_t s_fs;
static uint32_t       s_clear_window_ms;

/* Fast loop 카운터 */
static uint8_t s_oc_cnt;
static uint8_t s_sum_cnt;

/* Fast loop 에서 매 샘플 파라미터 조회를 피하기 위해 캐시 (1kHz 갱신) */
static volatile int32_t s_oc_trip_mA  = PROT_PHASE_OC_TRIP_MA;
static volatile int32_t s_sum_trip_mA = PROT_CUR_SUM_TRIP_MA;

static inline int32_t Abs32(int32_t x) { return (x < 0) ? -x : x; }

void SVC_Fault_Init(void)
{
    memset(&s_fs, 0, sizeof(s_fs));
    s_fs.first_id = FAULT_ID_NUM;
    s_clear_window_ms = 0U;
}

void SVC_Fault_Set(Fault_Id_e id)
{
    if (id >= FAULT_ID_NUM)
    {
        return;
    }

    /* Critical 이면 먼저 출력부터 차단 (가장 빠른 경로) */
    if ((FAULT_BIT(id) & FAULT_CRITICAL_MASK) != 0UL)
    {
        BSP_PWM_OutputDisable();
    }

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if ((s_fs.active & FAULT_BIT(id)) == 0UL)
    {
        s_fs.count[id]++;
        if (s_fs.first_id == FAULT_ID_NUM)
        {
            s_fs.first_id      = id;
            s_fs.first_tick_ms = BSP_GetTickMs();
        }
    }
    s_fs.active  |= FAULT_BIT(id);
    s_fs.history |= FAULT_BIT(id);
    __set_PRIMASK(primask);
}

bool SVC_Fault_IsActive(Fault_Id_e id)
{
    return (s_fs.active & FAULT_BIT(id)) != 0UL;
}

bool SVC_Fault_HasCritical(void)
{
    return (s_fs.active & FAULT_CRITICAL_MASK) != 0UL;
}

uint32_t SVC_Fault_GetActiveMask(void)
{
    return s_fs.active;
}

const Fault_Status_t *SVC_Fault_GetStatus(void)
{
    return &s_fs;
}

bool SVC_Fault_RequestClear(void)
{
    if (s_fs.perm_lock)
    {
        return false;
    }

    /* HW 원인이 아직 남아 있으면 해제 불가 */
    if (BSP_DRV_IsFault())
    {
        return false;
    }
    BSP_PWM_ClearBreak();
    if (BSP_PWM_IsBreakActive())
    {
        return false;
    }

    s_fs.clear_cnt++;
    if (s_fs.clear_cnt > PROT_FAULT_CLEAR_MAX)
    {
        s_fs.perm_lock = true;      /* 반복 트립 → 영구 래치 */
        return false;
    }
    s_clear_window_ms = FAULT_CLEAR_WINDOW_MS;

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    s_fs.active   = 0UL;            /* 지속 조건은 다음 검사 주기에 다시 set 된다 */
    s_fs.first_id = FAULT_ID_NUM;
    s_oc_cnt  = 0U;
    s_sum_cnt = 0U;
    __set_PRIMASK(primask);
    return true;
}

/* ------------------------------------------------------------------------ */
/* 20kHz Fast protection (ADC ISR context)                                   */
/* ------------------------------------------------------------------------ */
void SVC_Fault_CheckFast(int32_t iu_mA, int32_t iv_mA, int32_t iw_mA, bool sum_check_valid)
{
    const int32_t trip = s_oc_trip_mA;

    /* 1) 상전류 순시 과전류: 연속 N 샘플 (노이즈 1샘플 무시) */
    if ((Abs32(iu_mA) > trip) || (Abs32(iv_mA) > trip) || (Abs32(iw_mA) > trip))
    {
        if (++s_oc_cnt >= PROT_PHASE_OC_COUNT)
        {
            SVC_Fault_Set(FAULT_PHASE_OC);
            s_oc_cnt = PROT_PHASE_OC_COUNT;
        }
    }
    else
    {
        s_oc_cnt = 0U;
    }

    /* 2) 3상 합: Y결선 중성점 비접지 → Ia+Ib+Ic = 0 이어야 함.
     *    low-side shunt 는 듀티가 높으면 일부 상 측정 불가 → 상위에서 유효할 때만 검사 */
    if (sum_check_valid)
    {
        if (Abs32(iu_mA + iv_mA + iw_mA) > s_sum_trip_mA)
        {
            if (++s_sum_cnt >= PROT_PHASE_OC_COUNT)
            {
                SVC_Fault_Set(FAULT_CUR_SUM);
                s_sum_cnt = PROT_PHASE_OC_COUNT;
            }
        }
        else
        {
            s_sum_cnt = 0U;
        }
    }
}

void SVC_Fault_Task1ms(void)
{
    /* 파라미터 캐시 갱신 */
    s_oc_trip_mA  = SVC_Param_Get(PARAM_OC_TRIP_MA);
    s_sum_trip_mA = SVC_Param_Get(PARAM_CUR_SUM_TRIP_MA);

    /* nFAULT 레벨 폴링 (Break 인터럽트의 보조 경로) */
    if (BSP_DRV_IsEnabled() && BSP_DRV_IsFault())
    {
        SVC_Fault_Set(FAULT_DRV_NFAULT);
    }

    /* 클리어 횟수 윈도우 */
    if (s_clear_window_ms > 0U)
    {
        if (--s_clear_window_ms == 0U)
        {
            s_fs.clear_cnt = 0U;
        }
    }
}

/* BSP hook 재정의: TIM1 Break(nFAULT) — HW 가 이미 출력 차단함 */
void BSP_PWM_OnBreak(void)
{
    SVC_Fault_Set(FAULT_DRV_NFAULT);
}
