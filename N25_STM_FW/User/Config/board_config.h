/**
 ******************************************************************************
 * @file    board_config.h
 * @brief   N25 STM32G474 보드 하드웨어 상수 / 스케일 / 기능 스위치
 *
 * - 하드웨어(회로도)에서 결정되는 값은 모두 여기에만 둔다.
 * - 단위는 매크로 이름 끝에 붙인다: _HZ, _NS, _MV, _MA, _MOHM, _DEGC_X10 ...
 * - "TODO(HW)" 표시는 회로도/데이터시트로 최종 확인이 필요한 값.
 ******************************************************************************
 */
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <stdint.h>

/* ========================================================================== */
/* 1. Clock                                                                   */
/* ========================================================================== */
#define BOARD_HSE_HZ                (24000000UL)    /* Y1 24MHz oscillator (bypass) */
#define BOARD_SYSCLK_HZ             (170000000UL)   /* PLL: 24/6*85/2 */
#define BOARD_TIM1_CLK_HZ           (170000000UL)

/* ========================================================================== */
/* 2. PWM (TIM1, center-aligned)                                              */
/* ========================================================================== */
#define PWM_FREQ_HZ                 (20000UL)
/* center-aligned: f_pwm = f_tim / (2 * ARR) */
#define PWM_ARR                     ((uint16_t)(BOARD_TIM1_CLK_HZ / (2UL * PWM_FREQ_HZ)))  /* 4250 */
#define PWM_DEADTIME_NS             (300U)          /* MCU dead-time. DRV8353 DEAD_TIME 별도 설정 */
#define PWM_ADC_TRIG_OFFSET         (1U)            /* CCR4 = ARR - offset (카운터 peak 부근에서 샘플) */
#define PWM_DUTY_MAX_PERMIL         (950U)          /* 부트스트랩/최소 low-side on-time 확보 (TODO 튜닝) */
#define CTRL_FAST_LOOP_HZ           PWM_FREQ_HZ     /* ADC 인터럽트 = 20kHz */

/* ========================================================================== */
/* 3. Current sensing (DRV8353S CSA + low-side shunt)                         */
/* ========================================================================== */
#define ADC_VREF_MV                 (3300U)
#define ADC_FULL_SCALE              (4096U)
#define CUR_SHUNT_UOHM              (1000U)         /* R38/R39/R40 TLR3APDTE1L00 = 1.00mΩ */
#define CUR_CSA_GAIN                (20U)           /* DRV8353 CSA_GAIN: 5/10/20/40 (dev_drv8353 설정과 일치시킬 것) */
/* mA/LSB = (3300mV/4096) / (G * R[Ω]) = 0.80566mV / (20*0.001) = 40.28 mA/LSB          */
/* Q16 고정소수점: 40.283 * 65536 = 2,640,000                                               */
#define CUR_MA_PER_LSB_Q16          ((int32_t)((ADC_VREF_MV * 65536ULL * 1000000ULL) / \
                                     (ADC_FULL_SCALE * (uint64_t)CUR_CSA_GAIN * CUR_SHUNT_UOHM)))
#define CUR_OFFSET_NOMINAL_LSB      (2048)          /* VREF/2 (bidirectional) */
#define CUR_OFFSET_TOLERANCE_LSB    (150)           /* 오프셋 이상 판정 범위 */
#define CUR_OFFSET_CAL_SHIFT        (10U)           /* 2^10 = 1024 샘플 평균 */

/* ========================================================================== */
/* 4. Vbus sensing (R49 200k + R54 270k / R50 17.4k)                          */
/* ========================================================================== */
#define VBUS_R_TOP_OHM              (470000UL)
#define VBUS_R_BOT_OHM              (17400UL)
/* mV/LSB = 0.80566 * (487.4/17.4) = 22.57 mV/LSB, Full-scale ≈ 92.4V */
#define VBUS_MV_PER_LSB_Q16         ((int32_t)(((uint64_t)ADC_VREF_MV * 65536ULL * (VBUS_R_TOP_OHM + VBUS_R_BOT_OHM)) / \
                                     ((uint64_t)ADC_FULL_SCALE * VBUS_R_BOT_OHM)))

/* ========================================================================== */
/* 5. Temperature sensing (NTC + pull-up)                                     */
/*    !! 현재 회로: pull-up이 +5V → 3.3V 초과 구간 포화 (HW 리뷰 참고)          */
/* ========================================================================== */
#define NTC_PULLUP_MV               (5000U)         /* TODO(HW): 3.3V로 변경 시 3300 */
#define NTC_BOARD_PULLUP_OHM        (3300U)         /* R56 */
#define NTC_BOARD_R25_OHM           (2200U)         /* RT1 B57421V2222J062 = 2.2kΩ  TODO(HW) B값 확인 */
#define NTC_BOARD_BETA              (3560U)
#define NTC_MOTOR_PULLUP_OHM        (3300U)         /* R76 */
#define NTC_MOTOR_R25_OHM           (10000U)        /* 회로도 주석: 10kΩ, B3435 */
#define NTC_MOTOR_BETA              (3435U)

/* ========================================================================== */
/* 6. Encoder (iC-MU150 x2)                                                   */
/* ========================================================================== */
#define ENC_CNT_PER_REV             (16384U)        /* ABZ x4 = 16384 cnt/rev (N25와 동일) */
#define ENC_MU_POS_BITS             (17U)           /* SPI 절대값 유효 비트 (MU150 설정 의존, N25 기준) */
#define ENC_MU_TO_CNT_SHIFT         (ENC_MU_POS_BITS - 14U)   /* 17bit → 14bit(16384) */
#define MOTOR_POLE_PAIRS_DEFAULT    (7U)

/* ========================================================================== */
/* 7. Protection default thresholds (파라미터로 덮어씀)                         */
/* ========================================================================== */
#define PROT_PHASE_OC_TRIP_MA       (60000)         /* SW 순시 과전류 (상전류 피크) TODO: 모터 정격 기준 */
#define PROT_PHASE_OC_COUNT         (3U)            /* 연속 3샘플 = 150us */
#define PROT_CUR_SUM_TRIP_MA        (8000)          /* |Ia+Ib+Ic| 이상 (지락/센서 고장) */
#define PROT_VBUS_OV_TRIP_MV        (60000)         /* SMBJ58A 항복(64V) 이전 */
#define PROT_VBUS_UV_TRIP_MV        (36000)
#define PROT_TEMP_BOARD_TRIP_X10    (900)           /* 90.0°C */
#define PROT_TEMP_MOTOR_TRIP_X10    (1200)          /* 120.0°C TODO: 코일 절연/자석 등급 */
#define PROT_FAULT_CLEAR_MAX        (3U)            /* 60초 내 클리어 허용 횟수 (초과 시 영구 래치) */

/* ========================================================================== */
/* 8. CAN                                                                     */
/* ========================================================================== */
#define CAN_NODE_ID_DEFAULT         (1U)
#define CAN_BITRATE_KBPS            (1000U)         /* 1Mbps classic CAN */

/* ========================================================================== */
/* 9. EEPROM (M95128: 16KB, 64B page)                                         */
/* ========================================================================== */
#define EEPROM_SIZE_BYTES           (16384U)
#define EEPROM_PAGE_BYTES           (64U)
#define EEPROM_ADDR_PARAM_BANK_A    (0x0000U)
#define EEPROM_ADDR_PARAM_BANK_B    (0x0400U)
#define EEPROM_ADDR_FAULT_LOG       (0x1000U)

/* ========================================================================== */
/* 10. Feature switch                                                         */
/* ========================================================================== */
#define FEATURE_IWDG_ENABLE         (1)
#define FEATURE_BRINGUP_CMDS        (1)             /* 양산 시 0: 위험한 테스트 명령 제거 */

#endif /* BOARD_CONFIG_H */
