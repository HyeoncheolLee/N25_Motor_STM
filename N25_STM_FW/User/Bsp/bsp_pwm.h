/**
 ******************************************************************************
 * @file    bsp_pwm.h
 * @brief   3상 인버터 PWM (TIM1, center-aligned, complementary + dead-time)
 *
 *   TIM1 CH1/CH1N = U (PA8 / PB13)
 *   TIM1 CH2/CH2N = V (PA9 / PB14)
 *   TIM1 CH3/CH3N = W (PA10/ PB15)
 *   TIM1 CH4 (출력 없음) = ADC 트리거 시점 (TRGO2 = OC4REF)
 *   TIM1 BKIN (PC13)   = DRV8353 nFAULT → 하드웨어 PWM 즉시 차단
 *
 *   출력 허용은 MOE(Main Output Enable) 비트로 제어한다.
 *   - BSP_PWM_OutputEnable()  : MOE = 1
 *   - BSP_PWM_OutputDisable() : MOE = 0  → 6개 출력 모두 Low (모든 FET OFF)
 *   - Break(nFAULT Low) 발생 시 HW 가 MOE = 0 으로 만든다 (MCU 판단 없이 수십 ns)
 ******************************************************************************
 */
#ifndef BSP_PWM_H
#define BSP_PWM_H

#include <stdint.h>
#include <stdbool.h>

void     BSP_PWM_Init(void);                 /* 타이머 시작 (MOE=0 상태로) */
void     BSP_PWM_OutputEnable(void);
void     BSP_PWM_OutputDisable(void);        /* ISR 에서도 호출 가능 */
bool     BSP_PWM_IsOutputEnabled(void);

/* 듀티: 0 ~ PWM_ARR (compare 값 그대로). 범위 제한 포함 */
void     BSP_PWM_SetCompare(uint16_t cmp_u, uint16_t cmp_v, uint16_t cmp_w);
/* 듀티: 0 ~ 1000 permil (bring-up 테스트용) */
void     BSP_PWM_SetDutyPermil(uint16_t u, uint16_t v, uint16_t w);
uint16_t BSP_PWM_GetPeriod(void);

/* Break(nFAULT) 이벤트 */
bool     BSP_PWM_IsBreakActive(void);        /* Break 래치 플래그 */
void     BSP_PWM_ClearBreak(void);           /* nFAULT 해제 후에만 성공 */
uint32_t BSP_PWM_GetBreakCount(void);

/* Break 발생 시 호출되는 hook (weak). 상위(svc_fault)에서 재정의 */
void     BSP_PWM_OnBreak(void);

#endif /* BSP_PWM_H */
