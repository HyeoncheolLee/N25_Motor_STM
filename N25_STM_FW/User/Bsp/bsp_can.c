/**
 * @file    bsp_can.c
 */
#include "bsp_can.h"
#include "util_ringbuf.h"
#include "fdcan.h"          /* CubeMX: hfdcan1 */
#include <string.h>

#define CAN_RX_QUEUE_LEN    (32U)       /* 2의 거듭제곱 */
#define CAN_TX_QUEUE_LEN    (32U)

static BSP_CanMsg_t     s_rx_buf[CAN_RX_QUEUE_LEN];
static BSP_CanMsg_t     s_tx_buf[CAN_TX_QUEUE_LEN];
static RingBuf_Handle_t s_rx_q;
static RingBuf_Handle_t s_tx_q;
static BSP_CanStat_t    s_stat;

/* HAL 버전에 따라 FDCAN_DLC_BYTES_x 값이 (n) 또는 (n<<16) 이므로 표로 변환 */
static const uint32_t s_dlc_code[9] =
{
    FDCAN_DLC_BYTES_0, FDCAN_DLC_BYTES_1, FDCAN_DLC_BYTES_2, FDCAN_DLC_BYTES_3,
    FDCAN_DLC_BYTES_4, FDCAN_DLC_BYTES_5, FDCAN_DLC_BYTES_6, FDCAN_DLC_BYTES_7,
    FDCAN_DLC_BYTES_8
};

static uint8_t CAN_DlcCodeToLen(uint32_t code)
{
    if (code > 0xFU) { code >>= 16; }   /* 구버전 HAL */
    return (code > 8U) ? 8U : (uint8_t)code;
}

static bool CAN_WriteHw(const BSP_CanMsg_t *msg)
{
    FDCAN_TxHeaderTypeDef h = {0};

    if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) == 0U)
    {
        return false;
    }
    h.Identifier          = msg->id;
    h.IdType              = FDCAN_STANDARD_ID;
    h.TxFrameType         = FDCAN_DATA_FRAME;
    h.DataLength          = s_dlc_code[(msg->dlc > 8U) ? 8U : msg->dlc];
    h.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    h.BitRateSwitch       = FDCAN_BRS_OFF;
    h.FDFormat            = FDCAN_CLASSIC_CAN;
    h.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
    h.MessageMarker       = 0U;

    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &h, (uint8_t *)msg->data) != HAL_OK)
    {
        return false;
    }
    s_stat.tx_cnt++;
    return true;
}

void BSP_CAN_Init(uint16_t filter_id_lo, uint16_t filter_id_hi)
{
    FDCAN_FilterTypeDef f = {0};

    RingBuf_Init(&s_rx_q, s_rx_buf, sizeof(BSP_CanMsg_t), CAN_RX_QUEUE_LEN);
    RingBuf_Init(&s_tx_q, s_tx_buf, sizeof(BSP_CanMsg_t), CAN_TX_QUEUE_LEN);
    memset(&s_stat, 0, sizeof(s_stat));

    /* 필터 0: ID 범위 [lo, hi] → RX FIFO0  (CubeMX: Std Filters Nbr >= 1) */
    f.IdType       = FDCAN_STANDARD_ID;
    f.FilterIndex  = 0U;
    f.FilterType   = FDCAN_FILTER_RANGE;
    f.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    f.FilterID1    = filter_id_lo;
    f.FilterID2    = filter_id_hi;
    HAL_FDCAN_ConfigFilter(&hfdcan1, &f);

    /* 필터에 안 맞는 프레임은 버림 */
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT,
                                 FDCAN_REJECT_REMOTE, FDCAN_REJECT_REMOTE);

    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0U);
    HAL_FDCAN_Start(&hfdcan1);
}

/* main loop 전용 (TX 큐는 단일 생산자) */
bool BSP_CAN_Send(const BSP_CanMsg_t *msg)
{
    /* 큐가 비어 있으면 바로 HW 로 (지연 최소화) */
    if ((RingBuf_Count(&s_tx_q) == 0U) && CAN_WriteHw(msg))
    {
        return true;
    }
    if (!RingBuf_Push(&s_tx_q, msg))
    {
        s_stat.tx_overflow++;
        return false;
    }
    return true;
}

bool BSP_CAN_Receive(BSP_CanMsg_t *msg)
{
    return RingBuf_Pop(&s_rx_q, msg);
}

void BSP_CAN_Process(void)
{
    BSP_CanMsg_t m;
    FDCAN_ProtocolStatusTypeDef ps;

    /* TX 큐 → HW FIFO (최대 3개 HW 버퍼) */
    while (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) > 0U)
    {
        if (!RingBuf_Pop(&s_tx_q, &m))
        {
            break;
        }
        (void)CAN_WriteHw(&m);
    }

    /* Bus-off 자동 복구: INIT 비트 클리어 → 128 x 11 recessive bit 후 재참여 */
    if (HAL_FDCAN_GetProtocolStatus(&hfdcan1, &ps) == HAL_OK)
    {
        if (ps.BusOff != 0U)
        {
            s_stat.bus_off_cnt++;
            CLEAR_BIT(hfdcan1.Instance->CCCR, FDCAN_CCCR_INIT);
        }
    }
    s_stat.rx_overflow = s_rx_q.overflow_cnt;
}

const BSP_CanStat_t *BSP_CAN_GetStat(void)
{
    return &s_stat;
}

/* ------------------------------------------------------------------------ */
/* HAL callback: RX FIFO0                                                    */
/* ------------------------------------------------------------------------ */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    FDCAN_RxHeaderTypeDef h;
    BSP_CanMsg_t m;

    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0U)
    {
        return;
    }
    while (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) > 0U)
    {
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &h, m.data) != HAL_OK)
        {
            break;
        }
        if (h.IdType != FDCAN_STANDARD_ID)
        {
            continue;
        }
        m.id  = (uint16_t)h.Identifier;
        m.dlc = CAN_DlcCodeToLen(h.DataLength);
        s_stat.rx_cnt++;
        (void)RingBuf_Push(&s_rx_q, &m);
    }
}
