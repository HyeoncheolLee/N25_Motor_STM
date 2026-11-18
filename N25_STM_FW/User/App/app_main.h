/**
 ******************************************************************************
 * @file    app_main.h
 * @brief   Application 진입점. CubeMX main.c 에서 두 함수만 호출한다.
 *
 *   main.c (CubeMX 생성) 의 USER CODE 영역:
 *     USER CODE BEGIN Includes :  #include "app_main.h"
 *     USER CODE BEGIN 2        :  App_Init();
 *     USER CODE BEGIN 3 (while):  App_Loop();
 ******************************************************************************
 */
#ifndef APP_MAIN_H
#define APP_MAIN_H

#include <stdint.h>

typedef enum
{
    APP_STATE_INIT = 0,
    APP_STATE_READY,        /* 출력 OFF, 명령 대기 */
    APP_STATE_BRINGUP,      /* bring-up 시험 모드 동작 중 */
    APP_STATE_RUN,          /* (다음 단계) 모터 제어 운전 */
    APP_STATE_FAULT         /* Critical fault 래치 */
} App_State_e;

/* 부팅 자가진단 결과 (비트 = 실패 항목) */
#define APP_BOOT_ERR_EEPROM     (1U << 0)
#define APP_BOOT_ERR_PARAM      (1U << 1)
#define APP_BOOT_ERR_DRV        (1U << 2)
#define APP_BOOT_ERR_ENC1       (1U << 3)
#define APP_BOOT_ERR_ENC2       (1U << 4)
#define APP_BOOT_ERR_CUR_OFFSET (1U << 5)

void        App_Init(void);
void        App_Loop(void);
App_State_e App_GetState(void);
uint32_t    App_GetBootErrors(void);

#endif /* APP_MAIN_H */
