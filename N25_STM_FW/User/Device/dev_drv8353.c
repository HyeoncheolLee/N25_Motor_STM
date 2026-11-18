/**
 * @file    dev_drv8353.c
 */
#include "dev_drv8353.h"
#include "bsp.h"

#define DRV8353_WAKE_DELAY_MS   (2U)        /* ENABLE 후 SPI 가능까지 (데이터시트 tWAKE ≤ 1ms) */
#define DRV8353_DATA_MASK       (0x07FFU)

static DRV8353_Status_t s_status;

/* 16bit 프레임을 8bit x 2 로 전송 (CS 는 16클럭 동안 Low 유지) */
static bool DRV_Transfer(uint16_t tx_word, uint16_t *rx_word)
{
    uint8_t tx[2] = { (uint8_t)(tx_word >> 8), (uint8_t)(tx_word & 0xFFU) };
    uint8_t rx[2] = { 0U, 0U };

    if (BSP_SPI_Transfer(BSP_SPI_DEV_DRV8353, tx, rx, 2U) != HAL_OK)
    {
        s_status.spi_err_cnt++;
        return false;
    }
    if (rx_word != 0)
    {
        *rx_word = (uint16_t)(((uint16_t)rx[0] << 8) | rx[1]);
    }
    return true;
}

bool DRV8353_ReadReg(uint8_t addr, uint16_t *val)
{
    uint16_t frame = (uint16_t)((1U << 15) | ((uint16_t)(addr & 0x0FU) << 11));
    uint16_t rx;

    if (!DRV_Transfer(frame, &rx))
    {
        return false;
    }
    *val = rx & DRV8353_DATA_MASK;
    return true;
}

bool DRV8353_WriteReg(uint8_t addr, uint16_t val)
{
    uint16_t frame = (uint16_t)(((uint16_t)(addr & 0x0FU) << 11) | (val & DRV8353_DATA_MASK));
    return DRV_Transfer(frame, 0);
}

/* 쓰기 후 다시 읽어 일치 확인 (설정 실패를 조용히 넘기지 않는다) */
static bool DRV_WriteVerify(uint8_t addr, uint16_t val, uint16_t mask)
{
    uint16_t rd = 0U;
    if (!DRV8353_WriteReg(addr, val) || !DRV8353_ReadReg(addr, &rd))
    {
        return false;
    }
    if ((rd & mask) != (val & mask))
    {
        s_status.verify_err_cnt++;
        return false;
    }
    return true;
}

void DRV8353_GetDefaultConfig(DRV8353_Config_t *cfg)
{
    /* 시작값. bring-up 에서 게이트 파형 / 링잉 보고 조정 (TODO) */
    cfg->csa_gain     = DRV8353_CSA_GAIN_20;    /* board_config.h CUR_CSA_GAIN 과 일치 */
    cfg->ocp_mode     = DRV8353_OCP_LATCH;
    cfg->vds_lvl      = 0x4U;                   /* ≈0.1V → 병렬 2개(≈0.8~1.3mΩ) 기준 약 75~125A */
    cfg->ocp_deglitch = 0x1U;                   /* 2us */
    cfg->dead_time    = 0x1U;                   /* 100ns (MCU dead-time 300ns 와 별도) */
    cfg->idrivep_hs   = 0x6U;
    cfg->idriven_hs   = 0x6U;
    cfg->idrivep_ls   = 0x6U;
    cfg->idriven_ls   = 0x6U;
    cfg->sen_lvl      = 0x3U;                   /* 1.0V: shunt 1mΩ 에서는 사실상 미사용 */
}

bool DRV8353_Init(const DRV8353_Config_t *cfg)
{
    bool ok = true;
    uint16_t v;

    s_status.initialized = false;

    BSP_DRV_Enable();
    BSP_DelayMs(DRV8353_WAKE_DELAY_MS);

    /* 0x02 Driver Control: 6x PWM, fault clear */
    v = (uint16_t)(DRV8353_DC_PWM_MODE_6X | DRV8353_DC_CLR_FLT);
    ok &= DRV8353_WriteReg(DRV8353_REG_DRIVER_CTRL, v);

    /* 0x03 Gate Drive HS: LOCK=011(unlock) | IDRIVEP | IDRIVEN */
    v = (uint16_t)((0x3U << 8) | ((cfg->idrivep_hs & 0xFU) << 4) | (cfg->idriven_hs & 0xFU));
    ok &= DRV_WriteVerify(DRV8353_REG_GATE_DRIVE_HS, v, 0x00FFU);

    /* 0x04 Gate Drive LS: CBC=1 | TDRIVE=11(4000ns) | IDRIVEP | IDRIVEN */
    v = (uint16_t)((1U << 10) | (0x3U << 8) | ((cfg->idrivep_ls & 0xFU) << 4) | (cfg->idriven_ls & 0xFU));
    ok &= DRV_WriteVerify(DRV8353_REG_GATE_DRIVE_LS, v, 0x07FFU);

    /* 0x05 OCP Control: TRETRY=0 | DEAD_TIME | OCP_MODE | OCP_DEG | VDS_LVL */
    v = (uint16_t)(((cfg->dead_time & 0x3U) << 8) | (((uint16_t)cfg->ocp_mode & 0x3U) << 6) |
                   ((cfg->ocp_deglitch & 0x3U) << 4) | (cfg->vds_lvl & 0xFU));
    ok &= DRV_WriteVerify(DRV8353_REG_OCP_CTRL, v, 0x07FFU);

    /* 0x06 CSA Control: CSA_FET=0 | VREF_DIV=1(양방향) | LS_REF=0 | GAIN | DIS_SEN=0 | SEN_LVL */
    v = (uint16_t)((1U << 9) | (((uint16_t)cfg->csa_gain & 0x3U) << 6) | (cfg->sen_lvl & 0x3U));
    ok &= DRV_WriteVerify(DRV8353_REG_CSA_CTRL, v, 0x07FFU);

    /* 설정 후 Fault 상태 확인 */
    ok &= DRV8353_ReadFaults(0, 0);

    s_status.initialized = ok;
    return ok;
}

bool DRV8353_ReadFaults(uint16_t *fault1, uint16_t *vgs2)
{
    bool ok = DRV8353_ReadReg(DRV8353_REG_FAULT_STATUS1, &s_status.fault1) &&
              DRV8353_ReadReg(DRV8353_REG_VGS_STATUS2,   &s_status.vgs2);
    if (fault1 != 0) { *fault1 = s_status.fault1; }
    if (vgs2   != 0) { *vgs2   = s_status.vgs2; }
    return ok;
}

bool DRV8353_ClearFaults(void)
{
    uint16_t v;
    if (!DRV8353_ReadReg(DRV8353_REG_DRIVER_CTRL, &v))
    {
        return false;
    }
    return DRV8353_WriteReg(DRV8353_REG_DRIVER_CTRL, (uint16_t)(v | DRV8353_DC_CLR_FLT));
}

const DRV8353_Status_t *DRV8353_GetStatus(void)
{
    return &s_status;
}
