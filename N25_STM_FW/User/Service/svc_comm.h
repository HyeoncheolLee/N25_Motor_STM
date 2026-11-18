/**
 ******************************************************************************
 * @file    svc_comm.h
 * @brief   CAN 프로토콜 (전송 + 공통 명령 처리)
 *
 *  ID 배치 (CANopen 유사: 기능코드 + 노드ID, 11bit)
 *  ┌────────────────┬───────────┬──────────────────────────────────────────┐
 *  │ ID             │ 방향       │ 내용                                      │
 *  ├────────────────┼───────────┼──────────────────────────────────────────┤
 *  │ 0x600 + node   │ Host→Drv  │ 명령 (byte0 = CMD 코드)                   │
 *  │ 0x580 + node   │ Drv→Host  │ 명령 응답 (byte0 = CMD, byte1 = 결과)      │
 *  │ 0x180 + node   │ Drv→Host  │ TELEM_STATUS : fault mask, state, Vbus    │
 *  │ 0x280 + node   │ Drv→Host  │ TELEM_ENC    : ENC1 / ENC2 누적 카운트     │
 *  │ 0x380 + node   │ Drv→Host  │ TELEM_CUR    : Iu/Iv/Iw, 온도             │
 *  │ 0x480 + node   │ Drv→Host  │ TELEM_DIAG   : ISR 시간, overrun, 에러수   │
 *  │ 0x700 + node   │ Drv→Host  │ Heartbeat (1Hz)                           │
 *  └────────────────┴───────────┴──────────────────────────────────────────┘
 *  ※ N25(PIC32) 프로토콜과 다름. AMR 상위와 연동 단계에서 호환 계층 추가 예정.
 ******************************************************************************
 */
#ifndef SVC_COMM_H
#define SVC_COMM_H

#include <stdint.h>
#include <stdbool.h>
#include "bsp_can.h"

#define COMM_ID_CMD         (0x600U)
#define COMM_ID_RSP         (0x580U)
#define COMM_ID_TELEM_STAT  (0x180U)
#define COMM_ID_TELEM_ENC   (0x280U)
#define COMM_ID_TELEM_CUR   (0x380U)
#define COMM_ID_TELEM_DIAG  (0x480U)
#define COMM_ID_HEARTBEAT   (0x700U)

/* ---- 명령 코드 (byte0) ---------------------------------------------- */
typedef enum
{
    /* 공통 (svc_comm 처리) */
    CMD_PING            = 0x01,     /* → [ver u32][uid u16] */
    CMD_FAULT_GET       = 0x10,     /* → [active u32][first_id u8][perm_lock u8] */
    CMD_FAULT_CLEAR     = 0x11,
    CMD_PARAM_READ      = 0x20,     /* [id u8] → [id][val i32] */
    CMD_PARAM_WRITE     = 0x21,     /* [id u8][val i32] (RAM) */
    CMD_PARAM_SAVE      = 0x22,     /* EEPROM 저장 */
    CMD_PARAM_DEFAULT   = 0x23,     /* RAM 을 기본값으로 */
    CMD_RESET           = 0x7F,     /* [0xA5] 확인 바이트 */

    /* App 처리 (App_OnCommand 로 전달) */
    CMD_DRV_READ_REG    = 0x30,     /* [addr] → [addr][val u16] */
    CMD_DRV_WRITE_REG   = 0x31,     /* [addr][val u16] */
    CMD_DRV_ENABLE      = 0x32,     /* [0/1] */
    CMD_MU_READ_POS     = 0x40,     /* [ch] → [ch][pos u32][err][warn] */
    CMD_MU_READ_REG     = 0x41,     /* [ch][addr] → [ch][addr][val] */
    CMD_MU_WRITE_REG    = 0x42,     /* [ch][addr][val] */
    CMD_MU_BUS_RELEASE  = 0x43,     /* [1=release, 0=acquire] 외부 툴 설정용 */
    CMD_BU_MODE         = 0x50,     /* [mode] bring-up 모드 전환 */
    CMD_BU_PWM_DUTY     = 0x51,     /* [u u16][v u16][w u16] permil */
    CMD_BU_VECTOR       = 0x52,     /* [amp u16 permil][angle_deg u16] */
    CMD_BU_OPENLOOP     = 0x53,     /* [amp u16 permil][freq_e_x10 i16] */
    CMD_BU_CUR_CAL      = 0x54,     /* 전류 오프셋 재측정 */
    CMD_BRAKE           = 0x60,     /* [1=release, 0=engage] */
    CMD_STREAM          = 0x70,     /* [mask u8][period_ms u16] */
} Comm_Cmd_e;

typedef enum
{
    COMM_RES_OK          = 0,
    COMM_RES_ERR_ARG     = 1,       /* 인자 오류/범위 밖 */
    COMM_RES_ERR_STATE   = 2,       /* 현재 상태에서 불가 (Fault 등) */
    COMM_RES_ERR_UNKNOWN = 3,       /* 모르는 명령 */
    COMM_RES_ERR_HW      = 4,       /* SPI/EEPROM 등 HW 실패 */
} Comm_Result_e;

/* Stream mask */
#define COMM_STREAM_STATUS  (1U << 0)
#define COMM_STREAM_ENC     (1U << 1)
#define COMM_STREAM_CUR     (1U << 2)
#define COMM_STREAM_DIAG    (1U << 3)

void     SVC_Comm_Init(uint8_t node_id);
void     SVC_Comm_Task1ms(void);                 /* 수신 처리 */
bool     SVC_Comm_Send(uint16_t func_id, const uint8_t *data, uint8_t len);  /* func_id + node */
uint32_t SVC_Comm_GetMsSinceLastCmd(void);
uint8_t  SVC_Comm_GetNodeId(void);

/*
 * App 확장 hook (weak). App 계층에서 재정의.
 *   rx  : 수신 명령
 *   rsp : 응답 데이터 (rsp[0]=cmd, rsp[1]=result 는 svc_comm 이 채움, rsp[2..7] 만 작성)
 *   rsp_len : 응답 길이 (2~8)
 *   return : 결과 코드
 */
Comm_Result_e App_OnCommand(const BSP_CanMsg_t *rx, uint8_t *rsp, uint8_t *rsp_len);

/* 리틀엔디언 pack/unpack 헬퍼 */
static inline void     Comm_PutU16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static inline void     Comm_PutU32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static inline uint16_t Comm_GetU16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static inline uint32_t Comm_GetU32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

#endif /* SVC_COMM_H */
