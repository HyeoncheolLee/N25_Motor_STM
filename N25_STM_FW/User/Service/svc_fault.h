/**
 ******************************************************************************
 * @file    svc_fault.h
 * @brief   Fault 관리자 — 검출 / 래치 / 차단 / 기록 / 해제 정책
 *
 *  N25 에서 배운 점을 반영한 설계:
 *   1. 모든 보호 경로가 "하나의 함수(SVC_Fault_Set)"를 거친다 → 누락 방지
 *   2. Critical Fault 는 즉시 PWM 차단 (ISR 에서도 호출 가능)
 *   3. "최초 발생 Fault" 와 발생 시각을 기록 → 연쇄 에러 중 근본 원인 식별
 *   4. 짧은 시간에 반복 클리어 시 영구 래치 (Clear → 재기동 → 재트립 반복으로 HW 손상 방지)
 *   5. 계층: HW Break(nFAULT, ns) > Fast(20kHz, us) > Slow(1kHz, ms)
 ******************************************************************************
 */
#ifndef SVC_FAULT_H
#define SVC_FAULT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    /* --- Critical: 즉시 PWM 차단 ---------------------------------------- */
    FAULT_DRV_NFAULT = 0,       /* DRV8353 nFAULT → TIM1 Break (HW) */
    FAULT_PHASE_OC,             /* 상전류 순시 과전류 (20kHz SW) */
    FAULT_CUR_SUM,              /* |Ia+Ib+Ic| 이상 → 지락/센서/상 단선 */
    FAULT_VBUS_OV,              /* 과전압 (회생) */
    FAULT_VBUS_UV,              /* 저전압 */
    FAULT_TEMP_BOARD,           /* 보드 과열 */
    FAULT_TEMP_MOTOR,           /* 모터 과열 */
    FAULT_CUR_OFFSET,           /* 전류센서 오프셋 이상 (부팅 시) */
    FAULT_DRV_CONFIG,           /* DRV8353 SPI 설정/검증 실패 */
    FAULT_ENC1_SENSOR,          /* MU150 #1 nERR / 통신 실패 */
    FAULT_ENC_MISMATCH,         /* ABZ 누적값 vs SPI 절대값 불일치 (카운트 유실) */
    FAULT_CAN_TIMEOUT,          /* 지령 수신 끊김 (운전 중) */
    FAULT_FOLLOW_ERR,           /* (다음 단계) 추종오차 초과 */
    FAULT_I2T,                  /* (다음 단계) 열 누적 초과 */
    FAULT_CRITICAL_END,

    /* --- Warning: 기록만, 운전 지속 ------------------------------------- */
    FAULT_WARN_PARAM_DEFAULT = 16,  /* 파라미터 무효 → 기본값 사용 중 */
    FAULT_WARN_ENC2_SENSOR,     /* 보조 엔코더 이상 */
    FAULT_WARN_NTC_RANGE,       /* NTC 측정 범위 밖 (포화/단선 의심) */
    FAULT_WARN_WDG_RESET,       /* 직전 리셋이 watchdog */
    FAULT_WARN_SCHED_OVERRUN,   /* 1ms 태스크 지연 */
    FAULT_WARN_CAN_BUSOFF,
    FAULT_ID_NUM
} Fault_Id_e;

#define FAULT_BIT(id)           (1UL << (uint32_t)(id))
#define FAULT_CRITICAL_MASK     (FAULT_BIT(FAULT_CRITICAL_END) - 1UL)

typedef struct
{
    uint32_t   active;          /* 현재 래치된 fault 비트 */
    uint32_t   history;         /* 부팅 후 한 번이라도 발생한 비트 */
    Fault_Id_e first_id;        /* 최초 발생 fault (클리어 시 초기화) */
    uint32_t   first_tick_ms;
    uint32_t   count[FAULT_ID_NUM];
    uint8_t    clear_cnt;       /* 최근 60초 내 클리어 횟수 */
    bool       perm_lock;       /* 영구 래치 (전원 재인가 필요) */
} Fault_Status_t;

void  SVC_Fault_Init(void);
void  SVC_Fault_Set(Fault_Id_e id);            /* ISR-safe */
bool  SVC_Fault_IsActive(Fault_Id_e id);
bool  SVC_Fault_HasCritical(void);
uint32_t SVC_Fault_GetActiveMask(void);
const Fault_Status_t *SVC_Fault_GetStatus(void);

/* 해제 요청: 원인이 사라졌고 영구 래치가 아니면 true */
bool  SVC_Fault_RequestClear(void);

/* 20kHz Fast Loop 보호 (ISR) */
void  SVC_Fault_CheckFast(int32_t iu_mA, int32_t iv_mA, int32_t iw_mA, bool sum_check_valid);

/* 1kHz: 클리어 카운터 감쇠 등 */
void  SVC_Fault_Task1ms(void);

#endif /* SVC_FAULT_H */
