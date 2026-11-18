/**
 ******************************************************************************
 * @file    bsp.h
 * @brief   BSP 전체 초기화 / 공통 기능 (reset 원인, watchdog, tick)
 *
 *  BSP 계층 규칙:
 *   - STM32 HAL 을 직접 호출하는 유일한 계층 (상위 계층은 HAL 헤더 include 금지 지향)
 *   - 보드 의존 사항(핀, 타이머 번호, ADC 채널)을 상위에 숨긴다
 *   - 함수 이름: BSP_<모듈>_<동작>()   예) BSP_PWM_OutputEnable()
 ******************************************************************************
 */
#ifndef BSP_H
#define BSP_H

#include <stdint.h>
#include <stdbool.h>

#include "bsp_gpio.h"
#include "bsp_spi.h"
#include "bsp_pwm.h"
#include "bsp_adc.h"
#include "bsp_encoder.h"
#include "bsp_can.h"
#include "bsp_brake.h"

typedef enum
{
    BSP_RESET_POWER_ON = 0,
    BSP_RESET_PIN,
    BSP_RESET_SOFTWARE,
    BSP_RESET_IWDG,
    BSP_RESET_WWDG,
    BSP_RESET_BROWNOUT,
    BSP_RESET_OTHER
} BSP_ResetCause_e;

void             BSP_Init(void);             /* 안전 상태 → 주변장치 시작 (PWM 출력 OFF 유지) */
BSP_ResetCause_e BSP_GetResetCause(void);
void             BSP_WatchdogKick(void);
uint32_t         BSP_GetTickMs(void);
void             BSP_DelayMs(uint32_t ms);
void             BSP_SystemReset(void);
uint32_t         BSP_GetUid32(void);         /* MCU 고유 ID (96bit 를 32bit 로 접음) */

/* 1kHz 스케줄러 타이머(TIM6) 시작 — 마지막에 호출 */
void             BSP_StartSchedulerTimer(void);
/* TIM6 1ms 마다 호출되는 hook (weak, svc_sched 에서 재정의) */
void             BSP_OnTick1ms(void);

#endif /* BSP_H */
