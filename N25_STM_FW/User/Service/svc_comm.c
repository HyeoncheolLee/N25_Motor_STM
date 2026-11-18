/**
 * @file    svc_comm.c
 */
#include "svc_comm.h"
#include "svc_fault.h"
#include "svc_param.h"
#include "fw_version.h"
#include "bsp.h"
#include <string.h>

static uint8_t  s_node_id = 1U;
static uint32_t s_last_cmd_tick;

/* App 이 재정의하지 않으면 모르는 명령 */
__weak Comm_Result_e App_OnCommand(const BSP_CanMsg_t *rx, uint8_t *rsp, uint8_t *rsp_len)
{
    (void)rx; (void)rsp; (void)rsp_len;
    return COMM_RES_ERR_UNKNOWN;
}

void SVC_Comm_Init(uint8_t node_id)
{
    s_node_id = node_id;
    s_last_cmd_tick = BSP_GetTickMs();
    BSP_CAN_Init((uint16_t)(COMM_ID_CMD + node_id), (uint16_t)(COMM_ID_CMD + node_id));
}

uint8_t SVC_Comm_GetNodeId(void)
{
    return s_node_id;
}

bool SVC_Comm_Send(uint16_t func_id, const uint8_t *data, uint8_t len)
{
    BSP_CanMsg_t m;
    m.id  = (uint16_t)(func_id + s_node_id);
    m.dlc = (len > 8U) ? 8U : len;
    memcpy(m.data, data, m.dlc);
    return BSP_CAN_Send(&m);
}

uint32_t SVC_Comm_GetMsSinceLastCmd(void)
{
    return BSP_GetTickMs() - s_last_cmd_tick;
}

/* 공통 명령 처리. 처리했으면 true */
static bool Comm_HandleCommon(const BSP_CanMsg_t *rx, uint8_t *rsp, uint8_t *len, Comm_Result_e *res)
{
    const uint8_t *d = rx->data;

    switch ((Comm_Cmd_e)d[0])
    {
    case CMD_PING:
        Comm_PutU32(&rsp[2], FW_VERSION_U32);
        Comm_PutU16(&rsp[6], (uint16_t)BSP_GetUid32());
        *len = 8U;
        *res = COMM_RES_OK;
        return true;

    case CMD_FAULT_GET:
    {
        const Fault_Status_t *fs = SVC_Fault_GetStatus();
        Comm_PutU32(&rsp[2], fs->active);
        rsp[6] = (uint8_t)fs->first_id;
        rsp[7] = fs->perm_lock ? 1U : 0U;
        *len = 8U;
        *res = COMM_RES_OK;
        return true;
    }

    case CMD_FAULT_CLEAR:
        *res = SVC_Fault_RequestClear() ? COMM_RES_OK : COMM_RES_ERR_STATE;
        return true;

    case CMD_PARAM_READ:
        if ((rx->dlc < 2U) || (d[1] >= PARAM_ID_NUM)) { *res = COMM_RES_ERR_ARG; return true; }
        rsp[2] = d[1];
        Comm_PutU32(&rsp[3], (uint32_t)SVC_Param_Get((Param_Id_e)d[1]));
        *len = 7U;
        *res = COMM_RES_OK;
        return true;

    case CMD_PARAM_WRITE:
        if ((rx->dlc < 6U) || (d[1] >= PARAM_ID_NUM)) { *res = COMM_RES_ERR_ARG; return true; }
        *res = SVC_Param_Set((Param_Id_e)d[1], (int32_t)Comm_GetU32(&d[2])) ? COMM_RES_OK : COMM_RES_ERR_ARG;
        return true;

    case CMD_PARAM_SAVE:
        /* 저장 중 SPI 점유 ~수십 ms: 모터 정지 상태에서만 */
        if (BSP_PWM_IsOutputEnabled()) { *res = COMM_RES_ERR_STATE; return true; }
        *res = SVC_Param_Save() ? COMM_RES_OK : COMM_RES_ERR_HW;
        return true;

    case CMD_PARAM_DEFAULT:
        if (BSP_PWM_IsOutputEnabled()) { *res = COMM_RES_ERR_STATE; return true; }
        SVC_Param_SetDefault();
        *res = COMM_RES_OK;
        return true;

    case CMD_RESET:
        if ((rx->dlc >= 2U) && (d[1] == 0xA5U) && !BSP_PWM_IsOutputEnabled())
        {
            BSP_SystemReset();
        }
        *res = COMM_RES_ERR_STATE;
        return true;

    default:
        return false;
    }
}

void SVC_Comm_Task1ms(void)
{
    BSP_CanMsg_t  rx;
    uint8_t       rsp[8];
    uint8_t       len;
    Comm_Result_e res;

    /* 한 tick 에 최대 8개 처리 (main loop 독점 방지) */
    for (uint32_t n = 0U; (n < 8U) && BSP_CAN_Receive(&rx); n++)
    {
        if ((rx.id != (COMM_ID_CMD + s_node_id)) || (rx.dlc == 0U))
        {
            continue;
        }
        s_last_cmd_tick = BSP_GetTickMs();

        memset(rsp, 0, sizeof(rsp));
        len = 2U;
        res = COMM_RES_ERR_UNKNOWN;
        if (!Comm_HandleCommon(&rx, rsp, &len, &res))
        {
            res = App_OnCommand(&rx, rsp, &len);
        }
        rsp[0] = rx.data[0];
        rsp[1] = (uint8_t)res;
        if (len < 2U) { len = 2U; }
        (void)SVC_Comm_Send(COMM_ID_RSP, rsp, len);
    }
}
