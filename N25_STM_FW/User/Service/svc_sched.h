/**
 ******************************************************************************
 * @file    svc_sched.h
 * @brief   협조형(cooperative) 주기 태스크 스케줄러 — TIM6 1kHz 기준
 *
 *   ISR(TIM6) 는 "깃발만 세운다". 실제 태스크는 main loop 에서 실행.
 *   → ISR 를 짧게 유지, SPI/EEPROM 같은 느린 작업도 태스크에서 안전하게 수행
 *
 *   제어 루프 계층:
 *     20kHz : ADC ISR (Fast Loop)  — 전류 측정, 보호, (추후) FOC
 *      1kHz : TASK_1MS             — 모니터링, 통신 처리, (추후) 속도/위치 루프 지령
 *    100Hz  : TASK_10MS            — DRV 상태 읽기, 텔레메트리
 *     10Hz  : TASK_100MS           — LED, 엔코더 정합성 검사
 *      1Hz  : TASK_1000MS          — heartbeat
 ******************************************************************************
 */
#ifndef SVC_SCHED_H
#define SVC_SCHED_H

#include <stdint.h>

typedef enum
{
    SCHED_TASK_1MS = 0,
    SCHED_TASK_10MS,
    SCHED_TASK_100MS,
    SCHED_TASK_1000MS,
    SCHED_TASK_NUM
} Sched_Task_e;

typedef void (*Sched_TaskFunc_t)(void);

void     SVC_Sched_Init(void);
void     SVC_Sched_Register(Sched_Task_e task, Sched_TaskFunc_t func);
void     SVC_Sched_Run(void);                  /* main loop 에서 계속 호출 */
uint32_t SVC_Sched_GetTick(void);              /* 1ms tick */
uint32_t SVC_Sched_GetOverrunCount(void);      /* 1ms 태스크가 제때 못 돈 횟수 */

#endif /* SVC_SCHED_H */
