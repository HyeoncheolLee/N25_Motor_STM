/**
 ******************************************************************************
 * @file    svc_param.h
 * @brief   파라미터 관리 (EEPROM 이중 뱅크 + CRC16)
 *
 *  - 모든 튜닝값을 하나의 표(PARAM_LIST)로 관리: ID / 기본값 / 최소 / 최대
 *  - 저장 구조: [magic][layout][count][seq][val[0..N-1]][crc16]
 *    Bank A(0x0000), Bank B(0x0400) 에 번갈아 저장 → 저장 중 전원 차단에도 한쪽은 유효
 *  - 로드: 두 뱅크 중 CRC 유효 + seq 큰 쪽 선택. 둘 다 무효면 기본값
 *  - 새 파라미터는 PARAM_LIST 의 "맨 끝"에만 추가 (ID 순서 = EEPROM 위치)
 *
 *  사용 예)  int32_t lim = SVC_Param_Get(PARAM_CUR_LIMIT_MA);
 ******************************************************************************
 */
#ifndef SVC_PARAM_H
#define SVC_PARAM_H

#include <stdint.h>
#include <stdbool.h>

/*        ID                         default   min       max      설명                         */
#define PARAM_LIST(X) \
    X(PARAM_CAN_NODE_ID,              1,       1,        127)     /* CAN 노드 ID                */ \
    X(PARAM_MOTOR_DIR,                0,       0,        1)       /* 회전 방향 반전              */ \
    X(PARAM_POLE_PAIRS,               7,       1,        32)      /* 극쌍수                      */ \
    X(PARAM_ENC_ELEC_OFFSET_CNT,      0,       -16384,   16384)   /* 전기각 0 위치 (DQ align)    */ \
    X(PARAM_ENC_MISMATCH_MAX_CNT,     0,       0,        2000)    /* ABZ vs SPI 허용 차이. 0=감시 OFF (방향 확인 후 40≈6°e 권장) */ \
    X(PARAM_CUR_LIMIT_MA,             20000,   0,        80000)   /* 피크 전류 지령 한계          */ \
    X(PARAM_CUR_CONT_MA,              8000,    0,        40000)   /* 연속 정격 (I2t 기준)        */ \
    X(PARAM_OC_TRIP_MA,               60000,   1000,     80000)   /* SW 순시 과전류              */ \
    X(PARAM_CUR_SUM_TRIP_MA,          8000,    500,      40000)   /* |Ia+Ib+Ic| 이상             */ \
    X(PARAM_VBUS_OV_MV,               60000,   40000,    64000)   /* 과전압                      */ \
    X(PARAM_VBUS_UV_MV,               36000,   10000,    48000)   /* 저전압 (bring-up 저전압 시험 시 낮춤) */ \
    X(PARAM_TEMP_BOARD_TRIP_X10,      900,     300,      1250)    /* 보드 과열 [0.1°C]           */ \
    X(PARAM_TEMP_MOTOR_TRIP_X10,      1200,    300,      1500)    /* 모터 과열 [0.1°C]           */ \
    X(PARAM_FOLLOW_ERR_MAX_CNT,       4096,    100,      65535)   /* 추종오차 한계 (N25 windup 대응) */ \
    X(PARAM_DRV_VDS_LVL,              4,       0,        15)      /* DRV8353 VDS OCP 코드        */ \
    X(PARAM_DRV_IDRIVE_HS,            6,       0,        15)      \
    X(PARAM_DRV_IDRIVE_LS,            6,       0,        15)      \
    X(PARAM_CAN_TIMEOUT_MS,           100,     10,       5000)    /* 지령 미수신 → 정지          */ \
    X(PARAM_CUR_KP_X1000,             0,       0,        1000000) /* (다음 단계) 전류 PI         */ \
    X(PARAM_CUR_KI_X1000,             0,       0,        1000000) \
    X(PARAM_SPD_KP_X1000,             0,       0,        1000000) /* (다음 단계) 속도 PI         */ \
    X(PARAM_SPD_KI_X1000,             0,       0,        1000000) \
    X(PARAM_POS_KP_X1000,             0,       0,        1000000) /* (다음 단계) 위치 P          */

#define PARAM_ENUM_ITEM(id, def, min, max)  id,
typedef enum
{
    PARAM_LIST(PARAM_ENUM_ITEM)
    PARAM_ID_NUM
} Param_Id_e;
#undef PARAM_ENUM_ITEM

typedef enum
{
    PARAM_LOAD_OK_BANK_A = 0,
    PARAM_LOAD_OK_BANK_B,
    PARAM_LOAD_DEFAULT,         /* 두 뱅크 모두 무효 → 기본값 */
    PARAM_LOAD_EEPROM_ERR       /* EEPROM 통신 실패 → 기본값 */
} Param_LoadResult_e;

Param_LoadResult_e SVC_Param_Load(void);
bool               SVC_Param_Save(void);
void               SVC_Param_SetDefault(void);

int32_t            SVC_Param_Get(Param_Id_e id);
bool               SVC_Param_Set(Param_Id_e id, int32_t value);   /* 범위 검사 (RAM 만 변경) */
bool               SVC_Param_GetRange(Param_Id_e id, int32_t *min, int32_t *max);
bool               SVC_Param_IsDirty(void);                       /* 저장 안 된 변경 존재 */

#endif /* SVC_PARAM_H */
