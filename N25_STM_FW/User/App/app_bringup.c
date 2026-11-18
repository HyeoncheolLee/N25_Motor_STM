/**
 * @file    app_bringup.c
 */
#include "app_bringup.h"
#include "board_config.h"
#include "bsp.h"
#include "svc_fault.h"
#include "svc_comm.h"
#include <math.h>

#define BU_TWO_PI               (6.28318530718f)
#define BU_INV_SQRT3            (0.57735026919f)
#define BU_SUM_VALID_DUTY_MAX   (0.85f)     /* low-side on-time ≥ 15% 일 때만 3상 합 검사 */

static volatile BU_Mode_e s_mode = BU_MODE_OFF;
static volatile float     s_amp_norm;       /* 위상전압 피크 / Vdc  (0 ~ 0.577) */
static volatile float     s_theta;          /* 전기각 [rad] */
static volatile float     s_dtheta;         /* 20kHz 1 샘플당 증가량 */
static volatile bool      s_sum_valid = true;
static uint32_t           s_on_ms;

/* ------------------------------------------------------------------------ */
/* SVPWM (min-max zero-sequence injection)                                   */
/*   va,vb,vc = m·cos(θ), m·cos(θ-120°), m·cos(θ+120°)                       */
/*   voff = -(max + min)/2  → 듀티 = 0.5 + vx + voff                          */
/*   m ≤ 1/√3 에서 선형 (공간벡터 변조와 동일 결과)                             */
/* ------------------------------------------------------------------------ */
static void BU_ApplyVoltage(float m, float theta)
{
    float va = m * cosf(theta);
    float vb = m * cosf(theta - (BU_TWO_PI / 3.0f));
    float vc = m * cosf(theta + (BU_TWO_PI / 3.0f));

    float vmax = fmaxf(va, fmaxf(vb, vc));
    float vmin = fminf(va, fminf(vb, vc));
    float voff = -0.5f * (vmax + vmin);

    float da = 0.5f + va + voff;
    float db = 0.5f + vb + voff;
    float dc = 0.5f + vc + voff;

    s_sum_valid = (fmaxf(da, fmaxf(db, dc)) <= BU_SUM_VALID_DUTY_MAX);

    BSP_PWM_SetCompare((uint16_t)(da * (float)PWM_ARR),
                       (uint16_t)(db * (float)PWM_ARR),
                       (uint16_t)(dc * (float)PWM_ARR));
}

static bool BU_CanEnableOutput(void)
{
    return !SVC_Fault_HasCritical() &&
           BSP_DRV_IsEnabled() &&
           BSP_ADC_IsOffsetValid();
}

static void BU_StopOutput(void)
{
    BSP_PWM_OutputDisable();
    BSP_PWM_SetCompare(PWM_ARR / 2U, PWM_ARR / 2U, PWM_ARR / 2U);
    s_amp_norm  = 0.0f;
    s_dtheta    = 0.0f;
    s_sum_valid = true;
    s_mode      = BU_MODE_OFF;
}

void App_BU_Init(void)
{
    BU_StopOutput();
}

bool App_BU_SetMode(BU_Mode_e mode)
{
    if (mode >= BU_MODE_NUM)
    {
        return false;
    }
    if (mode == BU_MODE_OFF)
    {
        BU_StopOutput();
        return true;
    }
    if (!BU_CanEnableOutput())
    {
        return false;
    }

    /* 0V 상태(50%)로 출력 시작 → 이후 Set 명령으로 값 지정 */
    s_amp_norm = 0.0f;
    s_dtheta   = 0.0f;
    BSP_PWM_SetCompare(PWM_ARR / 2U, PWM_ARR / 2U, PWM_ARR / 2U);
    s_on_ms = 0U;
    s_mode  = mode;
    BSP_PWM_OutputEnable();
    return true;
}

BU_Mode_e App_BU_GetMode(void)
{
    return s_mode;
}

bool App_BU_SetDuty(uint16_t u, uint16_t v, uint16_t w)
{
    if (s_mode != BU_MODE_PWM_FIXED)
    {
        return false;
    }
    uint16_t lo = 500U - BU_DUTY_DELTA_MAX;
    uint16_t hi = 500U + BU_DUTY_DELTA_MAX;
    if ((u < lo) || (u > hi) || (v < lo) || (v > hi) || (w < lo) || (w > hi))
    {
        return false;
    }
    BSP_PWM_SetDutyPermil(u, v, w);
    return true;
}

bool App_BU_SetVector(uint16_t amp_permil, uint16_t angle_deg)
{
    if ((s_mode != BU_MODE_VECTOR) || (amp_permil > BU_AMP_MAX_PERMIL) || (angle_deg >= 360U))
    {
        return false;
    }
    s_theta    = (float)angle_deg * (BU_TWO_PI / 360.0f);
    s_amp_norm = ((float)amp_permil / 1000.0f) * BU_INV_SQRT3;
    return true;
}

bool App_BU_SetOpenLoop(uint16_t amp_permil, int16_t freq_e_x10)
{
    if ((s_mode != BU_MODE_OPENLOOP) || (amp_permil > BU_AMP_MAX_PERMIL) ||
        (freq_e_x10 > BU_FREQ_MAX_X10) || (freq_e_x10 < -BU_FREQ_MAX_X10))
    {
        return false;
    }
    s_amp_norm = ((float)amp_permil / 1000.0f) * BU_INV_SQRT3;
    s_dtheta   = BU_TWO_PI * ((float)freq_e_x10 / 10.0f) / (float)CTRL_FAST_LOOP_HZ;
    return true;
}

bool App_BU_IsCurrentSumValid(void)
{
    return s_sum_valid;
}

/* 20kHz (ADC ISR) */
void App_BU_FastLoop(void)
{
    switch (s_mode)
    {
    case BU_MODE_VECTOR:
        BU_ApplyVoltage(s_amp_norm, s_theta);
        break;

    case BU_MODE_OPENLOOP:
    {
        float th = s_theta + s_dtheta;
        if (th >= BU_TWO_PI) { th -= BU_TWO_PI; }
        if (th < 0.0f)       { th += BU_TWO_PI; }
        s_theta = th;
        BU_ApplyVoltage(s_amp_norm, th);
        break;
    }

    case BU_MODE_PWM_FIXED:     /* 듀티는 명령 시 직접 설정 */
    case BU_MODE_OFF:
    default:
        break;
    }
}

void App_BU_Task1ms(void)
{
    if (s_mode == BU_MODE_OFF)
    {
        return;
    }

    /* Critical fault 는 svc_fault 가 이미 MOE 를 껐다 → 모드도 정리 */
    if (SVC_Fault_HasCritical() ||
        (++s_on_ms > BU_MAX_ON_MS) ||
        (SVC_Comm_GetMsSinceLastCmd() > BU_HOST_TIMEOUT_MS))
    {
        BU_StopOutput();
    }
}
