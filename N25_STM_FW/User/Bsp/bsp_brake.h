/**
 ******************************************************************************
 * @file    bsp_brake.h
 * @brief   전자 브레이크 솔레노이드 (PB9 = TIM17_CH1 → MCP1416 → Q15)
 *
 *   무여자 작동형(전원 OFF = 제동) 브레이크 가정:
 *     Release: 흡인(pull-in) 듀티로 짧게 → 유지(hold) 듀티로 감소 (발열 저감)
 *     Engage : 듀티 0 → 스프링 제동
 *   N25 기존값: pull-in 50% / hold 23%, pull-in 150ms
 ******************************************************************************
 */
#ifndef BSP_BRAKE_H
#define BSP_BRAKE_H

#include <stdint.h>
#include <stdbool.h>

void BSP_Brake_Init(void);
void BSP_Brake_Release(void);       /* 모터 구동 전 */
void BSP_Brake_Engage(void);        /* 정지/Fault 시 */
bool BSP_Brake_IsReleased(void);
void BSP_Brake_Task1ms(void);       /* 1kHz: pull-in → hold 전환 */

#endif /* BSP_BRAKE_H */
