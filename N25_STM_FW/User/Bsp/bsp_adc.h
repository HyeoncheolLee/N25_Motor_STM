/**
 ******************************************************************************
 * @file    bsp_adc.h
 * @brief   ADC: 3상 전류(고속, 20kHz) + Vbus/온도(저속)
 *
 *  [고속] 3상 전류 동시 샘플링 — 3개의 ADC 를 같은 트리거(TIM1_TRGO2)로 시작
 *     ADC1 injected : W_CUR (PA2,  ADC1_IN3)
 *     ADC3 injected : U_CUR (PB1,  ADC3_IN1)
 *     ADC4 injected : V_CUR (PB12, ADC4_IN3)
 *     → ADC1 JEOS 인터럽트 = 20kHz 제어 ISR (Fast Loop)
 *
 *  [저속] ADC2 regular scan + DMA circular (연속 변환)
 *     rank1 : V_BAT      (PA4,  ADC2_IN17)
 *     rank2 : TEMP_BOARD (PB2,  ADC2_IN12)
 *     rank3 : TEMP_MOTOR (PB11, ADC2_IN14)
 *
 *  Low-side shunt 이므로 low-side FET 가 켜져 있을 때만 전류가 보인다.
 *  → 카운터 peak(모든 high-side OFF 구간 중앙)에서 샘플 (bsp_pwm 의 CCR4)
 ******************************************************************************
 */
#ifndef BSP_ADC_H
#define BSP_ADC_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    BSP_ADC_SLOW_VBUS = 0,
    BSP_ADC_SLOW_TEMP_BOARD,
    BSP_ADC_SLOW_TEMP_MOTOR,
    BSP_ADC_SLOW_NUM
} BSP_AdcSlowCh_e;

/* 한 번의 20kHz 샘플 결과 */
typedef struct
{
    uint16_t raw_u;         /* ADC raw (0~4095) */
    uint16_t raw_v;
    uint16_t raw_w;
    int32_t  iu_mA;         /* 오프셋 제거 + 스케일 적용 */
    int32_t  iv_mA;
    int32_t  iw_mA;
} BSP_PhaseCurrent_t;

typedef void (*BSP_ADC_FastCallback_t)(const BSP_PhaseCurrent_t *cur);

void     BSP_ADC_Init(void);                        /* 캘리브레이션 + 시작 */
void     BSP_ADC_RegisterFastCallback(BSP_ADC_FastCallback_t cb);

/* 전류 오프셋 캘리브레이션 (PWM 출력 OFF 상태에서 호출) */
void     BSP_ADC_StartOffsetCalib(void);
bool     BSP_ADC_IsOffsetCalibDone(void);
bool     BSP_ADC_IsOffsetValid(void);
void     BSP_ADC_GetOffset(int32_t *off_u, int32_t *off_v, int32_t *off_w);

uint16_t BSP_ADC_GetSlowRaw(BSP_AdcSlowCh_e ch);
const BSP_PhaseCurrent_t *BSP_ADC_GetLastCurrent(void);

/* ISR 진단 */
uint32_t BSP_ADC_GetIsrCount(void);
uint32_t BSP_ADC_GetIsrCyclesMax(void);             /* DWT 측정 최대 실행 사이클 */
void     BSP_ADC_ResetIsrStat(void);

#endif /* BSP_ADC_H */
