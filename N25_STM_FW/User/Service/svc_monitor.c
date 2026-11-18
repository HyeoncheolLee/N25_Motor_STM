/**
 * @file    svc_monitor.c
 */
#include "svc_monitor.h"
#include "svc_fault.h"
#include "svc_param.h"
#include "board_config.h"
#include "bsp.h"
#include "util_maf.h"
#include <math.h>

#define MON_VBUS_MAF_SHIFT      (3U)        /* N = 8  */
#define MON_TEMP_MAF_SHIFT      (6U)        /* N = 64 */
#define MON_VBUS_TRIP_COUNT     (3U)        /* 3ms 연속 */
#define MON_TEMP_PERIOD_MS      (10U)       /* 온도 변환은 100Hz 로 충분 */
#define MON_ADC_SAT_LSB         (4080U)     /* 이 이상이면 포화 */
#define MON_ADC_SHORT_LSB       (15U)       /* 이 이하면 단락 의심 */

static int32_t      s_vbus_buf[1U << MON_VBUS_MAF_SHIFT];
static int32_t      s_tb_buf[1U << MON_TEMP_MAF_SHIFT];
static int32_t      s_tm_buf[1U << MON_TEMP_MAF_SHIFT];
static MAF_Handle_t s_vbus_maf, s_tb_maf, s_tm_maf;

static Monitor_Data_t s_mon;
static uint8_t  s_ov_cnt, s_uv_cnt;
static uint8_t  s_temp_div;

/* NTC: Vadc = Vpu * Rntc / (Rpu + Rntc)  →  Rntc = Rpu * Vadc / (Vpu - Vadc)
 *      1/T = 1/T0 + ln(R/R0)/B                                                  */
static int32_t NTC_RawToTempX10(uint16_t raw, uint32_t r_pullup, uint32_t r25, uint32_t beta)
{
    float v_mV = (float)raw * (float)ADC_VREF_MV / (float)ADC_FULL_SCALE;
    float v_pu = (float)NTC_PULLUP_MV;
    if (v_mV >= (v_pu - 1.0f))
    {
        v_mV = v_pu - 1.0f;
    }
    float r_ntc = (float)r_pullup * v_mV / (v_pu - v_mV);
    if (r_ntc < 1.0f)
    {
        r_ntc = 1.0f;
    }
    float inv_t = (1.0f / 298.15f) + (logf(r_ntc / (float)r25) / (float)beta);
    float t_c   = (1.0f / inv_t) - 273.15f;
    return (int32_t)(t_c * 10.0f);
}

void SVC_Mon_Init(void)
{
    MAF_Init(&s_vbus_maf, s_vbus_buf, MON_VBUS_MAF_SHIFT);
    MAF_Init(&s_tb_maf,   s_tb_buf,   MON_TEMP_MAF_SHIFT);
    MAF_Init(&s_tm_maf,   s_tm_buf,   MON_TEMP_MAF_SHIFT);

    /* 첫 샘플로 버퍼를 채워 시작 램프 제거 (ADC DMA 가 이미 돌고 있어야 함) */
    MAF_Reset(&s_vbus_maf, BSP_ADC_GetSlowRaw(BSP_ADC_SLOW_VBUS));
    MAF_Reset(&s_tb_maf,   BSP_ADC_GetSlowRaw(BSP_ADC_SLOW_TEMP_BOARD));
    MAF_Reset(&s_tm_maf,   BSP_ADC_GetSlowRaw(BSP_ADC_SLOW_TEMP_MOTOR));
}

void SVC_Mon_Task1ms(void)
{
    /* ---------------- Vbus (1kHz) ---------------- */
    s_mon.raw_vbus = BSP_ADC_GetSlowRaw(BSP_ADC_SLOW_VBUS);
    int32_t vbus_lsb = MAF_Update(&s_vbus_maf, s_mon.raw_vbus);
    s_mon.vbus_mV = (int32_t)(((int64_t)vbus_lsb * VBUS_MV_PER_LSB_Q16) >> 16);

    if (s_mon.vbus_mV > SVC_Param_Get(PARAM_VBUS_OV_MV))
    {
        if (++s_ov_cnt >= MON_VBUS_TRIP_COUNT) { SVC_Fault_Set(FAULT_VBUS_OV); s_ov_cnt = MON_VBUS_TRIP_COUNT; }
    }
    else { s_ov_cnt = 0U; }

    if (s_mon.vbus_mV < SVC_Param_Get(PARAM_VBUS_UV_MV))
    {
        if (++s_uv_cnt >= MON_VBUS_TRIP_COUNT) { SVC_Fault_Set(FAULT_VBUS_UV); s_uv_cnt = MON_VBUS_TRIP_COUNT; }
    }
    else { s_uv_cnt = 0U; }

    /* ---------------- Temperature (MAF 는 1kHz, 변환은 100Hz) ---------------- */
    s_mon.raw_temp_board = BSP_ADC_GetSlowRaw(BSP_ADC_SLOW_TEMP_BOARD);
    s_mon.raw_temp_motor = BSP_ADC_GetSlowRaw(BSP_ADC_SLOW_TEMP_MOTOR);
    int32_t tb_lsb = MAF_Update(&s_tb_maf, s_mon.raw_temp_board);
    int32_t tm_lsb = MAF_Update(&s_tm_maf, s_mon.raw_temp_motor);

    if (++s_temp_div < MON_TEMP_PERIOD_MS)
    {
        return;
    }
    s_temp_div = 0U;

    s_mon.temp_board_x10 = NTC_RawToTempX10((uint16_t)tb_lsb, NTC_BOARD_PULLUP_OHM,
                                            NTC_BOARD_R25_OHM, NTC_BOARD_BETA);
    s_mon.temp_motor_x10 = NTC_RawToTempX10((uint16_t)tm_lsb, NTC_MOTOR_PULLUP_OHM,
                                            NTC_MOTOR_R25_OHM, NTC_MOTOR_BETA);

    /* pull-up 5V 회로: ADC 가 3.3V 에서 포화 → 모터 NTC 는 약 37°C 이하를 구분 못함 (HW 리뷰 #3)
     * 포화 = "저온" 으로 해석하고 보호 판정에서 제외 */
    s_mon.temp_motor_saturated = (tm_lsb >= (int32_t)MON_ADC_SAT_LSB);
    if ((tm_lsb <= (int32_t)MON_ADC_SHORT_LSB) || (tb_lsb <= (int32_t)MON_ADC_SHORT_LSB))
    {
        SVC_Fault_Set(FAULT_WARN_NTC_RANGE);    /* 단락 or 커넥터 이상 */
    }

    if (s_mon.temp_board_x10 > SVC_Param_Get(PARAM_TEMP_BOARD_TRIP_X10))
    {
        SVC_Fault_Set(FAULT_TEMP_BOARD);
    }
    if (!s_mon.temp_motor_saturated &&
        (s_mon.temp_motor_x10 > SVC_Param_Get(PARAM_TEMP_MOTOR_TRIP_X10)))
    {
        SVC_Fault_Set(FAULT_TEMP_MOTOR);
    }
}

const Monitor_Data_t *SVC_Mon_GetData(void)
{
    return &s_mon;
}

int32_t SVC_Mon_GetVbus_mV(void)
{
    return s_mon.vbus_mV;
}
