/**
 ******************************************************************************
 * @file    app_bringup.h
 * @brief   Bring-up 시험 모드 (CAN 명령으로 제어)
 *
 *  모드                    | 하는 일                                   | 확인 항목
 *  ------------------------|-------------------------------------------|-----------------------------
 *  BU_MODE_OFF             | PWM OFF                                   |
 *  BU_MODE_PWM_FIXED       | 3상 고정 듀티 (50% ± 15% 이내)              | 게이트 파형, 데드타임, 상 출력
 *  BU_MODE_VECTOR          | 고정 전압벡터 (크기, 전기각)                 | 회전자 정렬, 전류 부호, 전기각 오프셋
 *  BU_MODE_OPENLOOP        | 회전 전압벡터 (크기, 전기 주파수)            | 상순서, 극쌍수, 엔코더 방향
 *
 *  안전장치 (N25 교훈: 시험 중 코일 과열 방지)
 *   - 전압 크기 ≤ BU_AMP_MAX_PERMIL (선형 최대의 15%)
 *   - PWM 출력 상태 최대 BU_MAX_ON_MS 후 자동 OFF
 *   - Host 명령이 BU_HOST_TIMEOUT_MS 동안 없으면 자동 OFF
 *   - Critical fault 발생 시 즉시 OFF
 ******************************************************************************
 */
#ifndef APP_BRINGUP_H
#define APP_BRINGUP_H

#include <stdint.h>
#include <stdbool.h>

#define BU_AMP_MAX_PERMIL       (150U)
#define BU_DUTY_DELTA_MAX       (150U)      /* PWM_FIXED: 500 ± 150 permil */
#define BU_FREQ_MAX_X10         (500)       /* 50.0 Hz 전기각 = 약 430rpm @7극쌍 */
#define BU_MAX_ON_MS            (10000U)
#define BU_HOST_TIMEOUT_MS      (2000U)

typedef enum
{
    BU_MODE_OFF = 0,
    BU_MODE_PWM_FIXED,
    BU_MODE_VECTOR,
    BU_MODE_OPENLOOP,
    BU_MODE_NUM
} BU_Mode_e;

void      App_BU_Init(void);
bool      App_BU_SetMode(BU_Mode_e mode);       /* 진입 조건 불만족 시 false */
BU_Mode_e App_BU_GetMode(void);
bool      App_BU_SetDuty(uint16_t u, uint16_t v, uint16_t w);
bool      App_BU_SetVector(uint16_t amp_permil, uint16_t angle_deg);
bool      App_BU_SetOpenLoop(uint16_t amp_permil, int16_t freq_e_x10);

void      App_BU_FastLoop(void);                /* 20kHz ISR */
void      App_BU_Task1ms(void);
bool      App_BU_IsCurrentSumValid(void);       /* 듀티가 low-side 측정 가능 범위인지 */

#endif /* APP_BRINGUP_H */
