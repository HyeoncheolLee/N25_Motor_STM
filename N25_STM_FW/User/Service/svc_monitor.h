/**
 ******************************************************************************
 * @file    svc_monitor.h
 * @brief   저속 센싱 (Vbus, 보드/모터 온도) — Bit-shift MAF + 단위 변환 + 보호 판정
 *
 *   샘플링: 1kHz (ADC2 DMA 최신값을 가져옴)
 *   필터  : Vbus  MAF N=2^3=8   (지연 3.5ms)  — 과전압은 빨리 봐야 하므로 짧게
 *           Temp  MAF N=2^6=64  (지연 32ms)   — 열은 느리므로 노이즈 제거 우선
 *   보호  : OV/UV 는 필터값 + 연속 카운트, 온도는 필터값 + 히스테리시스
 ******************************************************************************
 */
#ifndef SVC_MONITOR_H
#define SVC_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    int32_t vbus_mV;
    int32_t temp_board_x10;     /* 0.1°C */
    int32_t temp_motor_x10;
    bool    temp_motor_saturated;   /* 회로 pull-up 5V 문제로 저온 구간 측정 불가 */
    uint16_t raw_vbus;
    uint16_t raw_temp_board;
    uint16_t raw_temp_motor;
} Monitor_Data_t;

void SVC_Mon_Init(void);
void SVC_Mon_Task1ms(void);
const Monitor_Data_t *SVC_Mon_GetData(void);
int32_t SVC_Mon_GetVbus_mV(void);

#endif /* SVC_MONITOR_H */
