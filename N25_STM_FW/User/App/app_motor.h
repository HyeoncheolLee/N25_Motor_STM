/**
 ******************************************************************************
 * @file    app_motor.h
 * @brief   [다음 단계] 모터 제어기 인터페이스 설계 초안 — 아직 구현 없음
 *
 *  Bring-up(G1~G6) 통과 후 구현한다. 예정 파일 구성:
 *
 *    App/app_motor.c      : 운전 상태 머신 (Align → Run → Stop), 지령 처리
 *    Ctrl/ctrl_foc.c/h    : Clarke/Park, 전류 PI (anti-windup 올바른 스케일), SVPWM, Vbus 보상
 *    Ctrl/ctrl_pi.c/h     : 범용 PI (출력 제한 + back-calculation / clamping)
 *    Ctrl/ctrl_speed.c/h  : 속도 추정(PLL/관측기) + 속도 PI
 *    Ctrl/ctrl_pos.c/h    : 위치 P + 추종오차 제한 (N25 windup 대응)
 *    Ctrl/ctrl_dob.c/h    : 외란 관측기 → 부하토크 추정 / 슬립 감지
 *    Ctrl/ctrl_i2t.c/h    : 코일 열 누적 보호
 *
 *  실행 위치:
 *    20kHz Fast Loop : App_Motor_FastLoop()   → 전류 루프 (FOC)
 *     1kHz Task      : App_Motor_Task1ms()    → 속도/위치 루프, I2t, 상태 머신
 *
 *  N25 이슈 → 설계 반영 포인트
 *    - 속도 루프 추가: 위치 P → 속도 PI → 전류 PI 캐스케이드
 *    - 속도 지령 적분 시 포화 감지 → 적분 정지 + 추종오차 클램프 (PARAM_FOLLOW_ERR_MAX_CNT)
 *    - 전류 한계 = min(PARAM_CUR_LIMIT_MA, I2t 허용치, 온도 디레이팅)
 *    - Vbus 실측으로 전압 한계/게인 보상, 회생 시 Vbus 상승 → 회생 토크 제한
 *    - 전기각: ABZ 누적 + MU150 절대값 주기 비교 (이미 bring-up 에 포함)
 ******************************************************************************
 */
#ifndef APP_MOTOR_H
#define APP_MOTOR_H

/* 인터페이스는 다음 단계에서 정의 */

#endif /* APP_MOTOR_H */
