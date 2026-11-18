/**
 ******************************************************************************
 * @file    bsp_can.h
 * @brief   FDCAN1 (Classic CAN 1Mbps) 송수신 래퍼
 *
 *   PA11 = FDCAN1_RX, PA12 = FDCAN1_TX, 트랜시버 TCAN1462 (STB = GND, 항상 Normal)
 *   수신: RX FIFO0 인터럽트 → 링버퍼 push → main loop 에서 BSP_CAN_Receive()로 pop
 *   송신: TX FIFO 여유 없으면 SW 큐에 넣고 BSP_CAN_Process()에서 재시도
 ******************************************************************************
 */
#ifndef BSP_CAN_H
#define BSP_CAN_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint16_t id;            /* 11bit standard ID */
    uint8_t  dlc;           /* 0~8 */
    uint8_t  data[8];
} BSP_CanMsg_t;

typedef struct
{
    uint32_t rx_cnt;
    uint32_t tx_cnt;
    uint32_t rx_overflow;
    uint32_t tx_overflow;
    uint32_t bus_off_cnt;
} BSP_CanStat_t;

void  BSP_CAN_Init(uint16_t filter_id_lo, uint16_t filter_id_hi); /* 수신 ID 범위 */
bool  BSP_CAN_Send(const BSP_CanMsg_t *msg);       /* main loop 전용 (ISR 에서 호출 금지) */
bool  BSP_CAN_Receive(BSP_CanMsg_t *msg);          /* main loop: 수신 1개 pop */
void  BSP_CAN_Process(void);                       /* main loop: TX 큐 → HW, bus-off 복구 */
const BSP_CanStat_t *BSP_CAN_GetStat(void);

#endif /* BSP_CAN_H */
