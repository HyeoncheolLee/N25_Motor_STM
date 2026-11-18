/**
 * @file    app_main.c
 * @brief   부팅 시퀀스, 상태 머신, 태스크 배치, Fast Loop, 텔레메트리, App 명령 처리
 */
#include "app_main.h"
#include "app_bringup.h"
#include "board_config.h"
#include "bsp.h"
#include "dev_drv8353.h"
#include "dev_mu150.h"
#include "dev_m95128.h"
#include "svc_sched.h"
#include "svc_fault.h"
#include "svc_param.h"
#include "svc_monitor.h"
#include "svc_comm.h"
#include "util_maf.h"
#include <stdlib.h>
#include <string.h>

/* ========================================================================== */
/* 내부 데이터                                                                 */
/* ========================================================================== */
#define TELEM_CUR_MAF_SHIFT     (5U)        /* 20kHz, N=32 → 1.6ms 평균 (표시용) */
#define CUR_OFFSET_CAL_TIMEOUT  (200U)      /* ms */

static App_State_e s_state = APP_STATE_INIT;
static uint32_t    s_boot_err;
static bool        s_spi_released = false;  /* 외부 툴이 MU150 설정 중 */

/* 텔레메트리용 전류 MAF (bit-shift) */
static int32_t      s_iu_buf[1U << TELEM_CUR_MAF_SHIFT];
static int32_t      s_iv_buf[1U << TELEM_CUR_MAF_SHIFT];
static int32_t      s_iw_buf[1U << TELEM_CUR_MAF_SHIFT];
static MAF_Handle_t s_iu_maf, s_iv_maf, s_iw_maf;

/* 스트리밍 */
static uint8_t  s_stream_mask   = COMM_STREAM_STATUS;
static uint16_t s_stream_period = 100U;
static uint16_t s_stream_cnt;

/* 엔코더 감시 */
static uint8_t  s_mu1_fail_cnt;
static int32_t  s_enc_diff_last;            /* ABZ vs SPI 차이 (디버그) */

/* ========================================================================== */
/* Fast Loop (20kHz, ADC ISR context)                                          */
/*  - 여기서는 SPI/CAN/EEPROM 호출 금지. 계산 + 레지스터 접근만.                  */
/* ========================================================================== */
static void App_FastLoop(const BSP_PhaseCurrent_t *cur)
{
    /* 1) 위치 갱신 */
    BSP_ENC_Update();

    /* 2) 보호 (항상 제어보다 먼저) */
    SVC_Fault_CheckFast(cur->iu_mA, cur->iv_mA, cur->iw_mA, App_BU_IsCurrentSumValid());

    /* 3) 표시용 필터 (제어에는 원시값 사용) */
    (void)MAF_Update(&s_iu_maf, cur->iu_mA);
    (void)MAF_Update(&s_iv_maf, cur->iv_mA);
    (void)MAF_Update(&s_iw_maf, cur->iw_mA);

    /* 4) 제어 */
    App_BU_FastLoop();
    /* (다음 단계) App_Motor_FastLoop(cur);  ← FOC 전류 제어 */
}

/* ========================================================================== */
/* 부팅 단계                                                                   */
/* ========================================================================== */
static void App_BootParam(void)
{
    if (!EEPROM_IsPresent())
    {
        s_boot_err |= APP_BOOT_ERR_EEPROM;
    }
    Param_LoadResult_e r = SVC_Param_Load();
    if ((r == PARAM_LOAD_DEFAULT) || (r == PARAM_LOAD_EEPROM_ERR))
    {
        s_boot_err |= APP_BOOT_ERR_PARAM;
        SVC_Fault_Set(FAULT_WARN_PARAM_DEFAULT);
    }
}

static void App_BootGateDriver(void)
{
    DRV8353_Config_t cfg;
    DRV8353_GetDefaultConfig(&cfg);
    cfg.vds_lvl    = (uint8_t)SVC_Param_Get(PARAM_DRV_VDS_LVL);
    cfg.idrivep_hs = cfg.idriven_hs = (uint8_t)SVC_Param_Get(PARAM_DRV_IDRIVE_HS);
    cfg.idrivep_ls = cfg.idriven_ls = (uint8_t)SVC_Param_Get(PARAM_DRV_IDRIVE_LS);

    /* DRV ENABLE → SPI 설정. PWM 출력(MOE)은 여전히 OFF */
    if (!DRV8353_Init(&cfg))
    {
        s_boot_err |= APP_BOOT_ERR_DRV;
        SVC_Fault_Set(FAULT_DRV_CONFIG);
    }
}

static void App_BootEncoder(void)
{
    MU150_Position_t p;

    /* ENC1: 절대값으로 ABZ 누적 카운터 초기화 (14bit = 16384 cnt/rev) */
    if (MU150_ReadPosition(MU150_CH1, &p) && !p.err)
    {
        BSP_ENC_SetCount(BSP_ENC_1, (int32_t)(p.pos_raw >> ENC_MU_TO_CNT_SHIFT));
    }
    else
    {
        s_boot_err |= APP_BOOT_ERR_ENC1;
        SVC_Fault_Set(FAULT_ENC1_SENSOR);
    }

    /* ENC2 (보조): 실패해도 warning */
    if (MU150_ReadPosition(MU150_CH2, &p) && !p.err)
    {
        BSP_ENC_SetCount(BSP_ENC_2, (int32_t)(p.pos_raw >> ENC_MU_TO_CNT_SHIFT));
    }
    else
    {
        s_boot_err |= APP_BOOT_ERR_ENC2;
        SVC_Fault_Set(FAULT_WARN_ENC2_SENSOR);
    }
}

static void App_BootCurrentOffset(void)
{
    /* 반드시 PWM 출력 OFF (전류 0) 상태에서 */
    BSP_PWM_OutputDisable();
    BSP_ADC_StartOffsetCalib();

    uint32_t t0 = BSP_GetTickMs();
    while (!BSP_ADC_IsOffsetCalibDone() && ((BSP_GetTickMs() - t0) < CUR_OFFSET_CAL_TIMEOUT))
    {
    }
    if (!BSP_ADC_IsOffsetValid())
    {
        s_boot_err |= APP_BOOT_ERR_CUR_OFFSET;
        SVC_Fault_Set(FAULT_CUR_OFFSET);
    }
}

/* ========================================================================== */
/* 주기 태스크                                                                 */
/* ========================================================================== */
static void App_UpdateState(void)
{
    if (SVC_Fault_HasCritical())
    {
        if (s_state != APP_STATE_FAULT)
        {
            App_BU_SetMode(BU_MODE_OFF);
            BSP_Brake_Engage();
            s_state = APP_STATE_FAULT;
        }
        return;
    }
    s_state = (App_BU_GetMode() != BU_MODE_OFF) ? APP_STATE_BRINGUP : APP_STATE_READY;
}

static void App_SendTelemetry(void)
{
    uint8_t d[8];

    if (s_stream_mask & COMM_STREAM_STATUS)
    {
        const Fault_Status_t *fs = SVC_Fault_GetStatus();
        Comm_PutU32(&d[0], fs->active);
        d[4] = (uint8_t)fs->first_id;
        d[5] = (uint8_t)s_state;
        Comm_PutU16(&d[6], (uint16_t)(SVC_Mon_GetVbus_mV() / 100));    /* 0.1V */
        (void)SVC_Comm_Send(COMM_ID_TELEM_STAT, d, 8U);
    }
    if (s_stream_mask & COMM_STREAM_ENC)
    {
        Comm_PutU32(&d[0], (uint32_t)BSP_ENC_GetCount(BSP_ENC_1));
        Comm_PutU32(&d[4], (uint32_t)BSP_ENC_GetCount(BSP_ENC_2));
        (void)SVC_Comm_Send(COMM_ID_TELEM_ENC, d, 8U);
    }
    if (s_stream_mask & COMM_STREAM_CUR)
    {
        const Monitor_Data_t *m = SVC_Mon_GetData();
        Comm_PutU16(&d[0], (uint16_t)(int16_t)(MAF_GetRounded(&s_iu_maf) / 10));   /* 10mA */
        Comm_PutU16(&d[2], (uint16_t)(int16_t)(MAF_GetRounded(&s_iv_maf) / 10));
        Comm_PutU16(&d[4], (uint16_t)(int16_t)(MAF_GetRounded(&s_iw_maf) / 10));
        d[6] = (uint8_t)(int8_t)(m->temp_board_x10 / 10);
        d[7] = (uint8_t)(int8_t)(m->temp_motor_x10 / 10);
        (void)SVC_Comm_Send(COMM_ID_TELEM_CUR, d, 8U);
    }
    if (s_stream_mask & COMM_STREAM_DIAG)
    {
        Comm_PutU16(&d[0], (uint16_t)BSP_ADC_GetIsrCyclesMax());        /* 8500 = 50us 한계 */
        Comm_PutU16(&d[2], (uint16_t)SVC_Sched_GetOverrunCount());
        Comm_PutU16(&d[4], (uint16_t)BSP_SPI_GetErrorCount());
        Comm_PutU16(&d[6], (uint16_t)s_enc_diff_last);
        (void)SVC_Comm_Send(COMM_ID_TELEM_DIAG, d, 8U);
    }
}

/* ABZ 누적 vs MU150 SPI 절대값 비교 → QEI 카운트 유실 감시 (N25 이슈 #4 대응) */
static void App_CheckEncoderConsistency(void)
{
    MU150_Position_t p;
    const int32_t max_diff = SVC_Param_Get(PARAM_ENC_MISMATCH_MAX_CNT);
    const int32_t half_rev = (int32_t)(ENC_CNT_PER_REV / 2U);

    int32_t c0 = BSP_ENC_GetCount(BSP_ENC_1);
    bool ok = MU150_ReadPosition(MU150_CH1, &p);
    int32_t c1 = BSP_ENC_GetCount(BSP_ENC_1);

    if (!ok)
    {
        if (++s_mu1_fail_cnt >= 3U) { SVC_Fault_Set(FAULT_ENC1_SENSOR); }
        return;
    }
    s_mu1_fail_cnt = 0U;
    if (p.err)
    {
        SVC_Fault_Set(FAULT_ENC1_SENSOR);
        return;
    }

    /* SPI 읽는 동안 많이 움직였으면 비교 생략 */
    if ((max_diff == 0) || (abs(c1 - c0) > max_diff))
    {
        return;
    }
    int32_t abz  = ((c0 + c1) / 2) & (int32_t)(ENC_CNT_PER_REV - 1U);
    int32_t absc = (int32_t)(p.pos_raw >> ENC_MU_TO_CNT_SHIFT);
    int32_t diff = ((absc - abz + half_rev) & (int32_t)(ENC_CNT_PER_REV - 1U)) - half_rev;   /* ±반바퀴로 wrap */

    s_enc_diff_last = diff;
    if (abs(diff) > max_diff)
    {
        SVC_Fault_Set(FAULT_ENC_MISMATCH);
    }
}

static void App_Task1ms(void)
{
    BSP_WatchdogKick();             /* main loop 가 멈추면 IWDG 리셋 */

    SVC_Fault_Task1ms();
    SVC_Mon_Task1ms();
    SVC_Comm_Task1ms();
    BSP_Brake_Task1ms();
    App_BU_Task1ms();
    App_UpdateState();

    if ((s_stream_period > 0U) && (++s_stream_cnt >= s_stream_period))
    {
        s_stream_cnt = 0U;
        App_SendTelemetry();
    }
}

static void App_Task10ms(void)
{
    /* DRV8353 Fault 레지스터 기록 (원인 분석용) */
    if (BSP_DRV_IsEnabled() && !s_spi_released)
    {
        (void)DRV8353_ReadFaults(0, 0);
    }
}

static void App_Task100ms(void)
{
    static uint8_t blink;
    blink++;

    switch (s_state)
    {
    case APP_STATE_FAULT:    BSP_LED_Toggle(BSP_LED_A); break;                     /* 5Hz */
    case APP_STATE_BRINGUP:  BSP_LED_On(BSP_LED_A);     break;                     /* 점등 */
    default:                 if ((blink % 5U) == 0U) { BSP_LED_Toggle(BSP_LED_A); } break; /* 1Hz */
    }

    if (!s_spi_released)
    {
        App_CheckEncoderConsistency();
    }
    if (SVC_Sched_GetOverrunCount() > 0U)
    {
        SVC_Fault_Set(FAULT_WARN_SCHED_OVERRUN);
    }
}

static void App_Task1000ms(void)
{
    uint8_t hb = (uint8_t)s_state;
    (void)SVC_Comm_Send(COMM_ID_HEARTBEAT, &hb, 1U);
}

/* ========================================================================== */
/* App 명령 처리 (svc_comm hook 재정의)                                        */
/* ========================================================================== */
Comm_Result_e App_OnCommand(const BSP_CanMsg_t *rx, uint8_t *rsp, uint8_t *rsp_len)
{
    const uint8_t *d = rx->data;
    const bool out_on = BSP_PWM_IsOutputEnabled();

    switch ((Comm_Cmd_e)d[0])
    {
    /* ---------------- DRV8353 ---------------- */
    case CMD_DRV_READ_REG:
    {
        uint16_t v;
        if (rx->dlc < 2U)                 { return COMM_RES_ERR_ARG; }
        if (!DRV8353_ReadReg(d[1], &v))   { return COMM_RES_ERR_HW; }
        rsp[2] = d[1];
        Comm_PutU16(&rsp[3], v);
        *rsp_len = 5U;
        return COMM_RES_OK;
    }
    case CMD_DRV_WRITE_REG:
        if (rx->dlc < 4U) { return COMM_RES_ERR_ARG; }
        if (out_on)       { return COMM_RES_ERR_STATE; }
        return DRV8353_WriteReg(d[1], Comm_GetU16(&d[2])) ? COMM_RES_OK : COMM_RES_ERR_HW;

    case CMD_DRV_ENABLE:
        if (rx->dlc < 2U) { return COMM_RES_ERR_ARG; }
        if (d[1] != 0U)   { App_BootGateDriver(); return DRV8353_GetStatus()->initialized ? COMM_RES_OK : COMM_RES_ERR_HW; }
        App_BU_SetMode(BU_MODE_OFF);
        BSP_DRV_Disable();
        return COMM_RES_OK;

    /* ---------------- iC-MU150 ---------------- */
    case CMD_MU_READ_POS:
    {
        MU150_Position_t p;
        if ((rx->dlc < 2U) || (d[1] >= MU150_CH_NUM))          { return COMM_RES_ERR_ARG; }
        if (!MU150_ReadPosition((MU150_Ch_e)d[1], &p))         { return COMM_RES_ERR_HW; }
        rsp[2] = d[1];
        Comm_PutU32(&rsp[3], p.pos_raw);
        rsp[7] = (uint8_t)((p.err ? 1U : 0U) | (p.warn ? 2U : 0U));
        *rsp_len = 8U;
        return COMM_RES_OK;
    }
    case CMD_MU_READ_REG:
    {
        uint8_t v;
        if ((rx->dlc < 3U) || (d[1] >= MU150_CH_NUM))              { return COMM_RES_ERR_ARG; }
        if (!MU150_ReadRegister((MU150_Ch_e)d[1], d[2], &v))       { return COMM_RES_ERR_HW; }
        rsp[2] = d[1]; rsp[3] = d[2]; rsp[4] = v;
        *rsp_len = 5U;
        return COMM_RES_OK;
    }
    case CMD_MU_WRITE_REG:
        if ((rx->dlc < 4U) || (d[1] >= MU150_CH_NUM)) { return COMM_RES_ERR_ARG; }
        if (out_on)                                   { return COMM_RES_ERR_STATE; }
        return MU150_WriteRegister((MU150_Ch_e)d[1], d[2], d[3]) ? COMM_RES_OK : COMM_RES_ERR_HW;

    case CMD_MU_BUS_RELEASE:
        if (rx->dlc < 2U) { return COMM_RES_ERR_ARG; }
        if (out_on)       { return COMM_RES_ERR_STATE; }
        if (d[1] != 0U) { BSP_SPI_ReleaseBus(); s_spi_released = true; }
        else            { BSP_SPI_AcquireBus(); s_spi_released = false; }
        return COMM_RES_OK;

    /* ---------------- Bring-up ---------------- */
    case CMD_BU_MODE:
        if ((rx->dlc < 2U) || (d[1] >= BU_MODE_NUM)) { return COMM_RES_ERR_ARG; }
        return App_BU_SetMode((BU_Mode_e)d[1]) ? COMM_RES_OK : COMM_RES_ERR_STATE;

    case CMD_BU_PWM_DUTY:
        if (rx->dlc < 7U) { return COMM_RES_ERR_ARG; }
        return App_BU_SetDuty(Comm_GetU16(&d[1]), Comm_GetU16(&d[3]), Comm_GetU16(&d[5])) ? COMM_RES_OK : COMM_RES_ERR_ARG;

    case CMD_BU_VECTOR:
        if (rx->dlc < 5U) { return COMM_RES_ERR_ARG; }
        return App_BU_SetVector(Comm_GetU16(&d[1]), Comm_GetU16(&d[3])) ? COMM_RES_OK : COMM_RES_ERR_ARG;

    case CMD_BU_OPENLOOP:
        if (rx->dlc < 5U) { return COMM_RES_ERR_ARG; }
        return App_BU_SetOpenLoop(Comm_GetU16(&d[1]), (int16_t)Comm_GetU16(&d[3])) ? COMM_RES_OK : COMM_RES_ERR_ARG;

    case CMD_BU_CUR_CAL:
        if (out_on) { return COMM_RES_ERR_STATE; }
        App_BootCurrentOffset();
        {
            int32_t ou, ov, ow;
            BSP_ADC_GetOffset(&ou, &ov, &ow);
            Comm_PutU16(&rsp[2], (uint16_t)ou);
            Comm_PutU16(&rsp[4], (uint16_t)ov);
            Comm_PutU16(&rsp[6], (uint16_t)ow);
            *rsp_len = 8U;
        }
        return BSP_ADC_IsOffsetValid() ? COMM_RES_OK : COMM_RES_ERR_HW;

    /* ---------------- Brake / Stream ---------------- */
    case CMD_BRAKE:
        if (rx->dlc < 2U) { return COMM_RES_ERR_ARG; }
        if (d[1] != 0U)
        {
            if (SVC_Fault_HasCritical()) { return COMM_RES_ERR_STATE; }
            BSP_Brake_Release();
        }
        else
        {
            BSP_Brake_Engage();
        }
        return COMM_RES_OK;

    case CMD_STREAM:
        if (rx->dlc < 4U) { return COMM_RES_ERR_ARG; }
        s_stream_mask   = d[1];
        s_stream_period = Comm_GetU16(&d[2]);       /* 0 = 중지 */
        if ((s_stream_period != 0U) && (s_stream_period < 5U)) { s_stream_period = 5U; }  /* CAN 부하 제한 */
        s_stream_cnt = 0U;
        return COMM_RES_OK;

    default:
        return COMM_RES_ERR_UNKNOWN;
    }
}

/* ========================================================================== */
/* Public                                                                      */
/* ========================================================================== */
void App_Init(void)
{
    s_state    = APP_STATE_INIT;
    s_boot_err = 0U;

    /* 1. 서비스 기본 초기화 (Fault 기록이 가장 먼저 가능해야 함) */
    SVC_Fault_Init();
    SVC_Sched_Init();

    /* 2. 하드웨어 안전 상태 + 주변장치 시작 (PWM 출력 OFF, ADC 20kHz 동작 시작) */
    BSP_Init();
    if (BSP_GetResetCause() == BSP_RESET_IWDG)
    {
        SVC_Fault_Set(FAULT_WARN_WDG_RESET);
    }

    /* 3. 표시용 MAF */
    MAF_Init(&s_iu_maf, s_iu_buf, TELEM_CUR_MAF_SHIFT);
    MAF_Init(&s_iv_maf, s_iv_buf, TELEM_CUR_MAF_SHIFT);
    MAF_Init(&s_iw_maf, s_iw_buf, TELEM_CUR_MAF_SHIFT);
    BSP_ADC_RegisterFastCallback(App_FastLoop);

    /* 4. 파라미터 → 5. 센서/드라이버 */
    /*    (IWDG 는 MX_IWDG_Init 에서 이미 동작 중 → 단계마다 kick) */
    App_BootParam();
    BSP_WatchdogKick();
    BSP_DelayMs(2U);                /* ADC2 DMA 가 몇 바퀴 돈 뒤 */
    SVC_Mon_Init();
    App_BootGateDriver();
    App_BootEncoder();
    BSP_WatchdogKick();
    App_BootCurrentOffset();        /* ~51ms (1024 샘플 @20kHz) */
    BSP_WatchdogKick();

    /* 6. 통신 */
    SVC_Comm_Init((uint8_t)SVC_Param_Get(PARAM_CAN_NODE_ID));

    /* 7. 태스크 등록 → 스케줄러 시작 */
    App_BU_Init();
    SVC_Sched_Register(SCHED_TASK_1MS,    App_Task1ms);
    SVC_Sched_Register(SCHED_TASK_10MS,   App_Task10ms);
    SVC_Sched_Register(SCHED_TASK_100MS,  App_Task100ms);
    SVC_Sched_Register(SCHED_TASK_1000MS, App_Task1000ms);
    BSP_StartSchedulerTimer();

    s_state = SVC_Fault_HasCritical() ? APP_STATE_FAULT : APP_STATE_READY;
}

void App_Loop(void)
{
    SVC_Sched_Run();
    BSP_CAN_Process();
}

App_State_e App_GetState(void)
{
    return s_state;
}

uint32_t App_GetBootErrors(void)
{
    return s_boot_err;
}
