/**
 * @file    dev_mu150.c
 */
#include "dev_mu150.h"
#include "bsp.h"

#define MU150_REG_BUSY_RETRY    (20U)
#define MU150_STATUS_BUSY_BIT   (0x02U)     /* N25: REG_STATUS 첫 바이트 bit1 = busy */

static MU150_Stat_t s_stat[MU150_CH_NUM];

static BSP_SpiDev_e MU_ToSpiDev(MU150_Ch_e ch)
{
    return (ch == MU150_CH1) ? BSP_SPI_DEV_MU1 : BSP_SPI_DEV_MU2;
}

bool MU150_ReadPosition(MU150_Ch_e ch, MU150_Position_t *pos)
{
    uint8_t tx[5] = { MU150_OP_SDAD, 0U, 0U, 0U, 0U };
    uint8_t rx[5] = { 0U };

    if (BSP_SPI_Transfer(MU_ToSpiDev(ch), tx, rx, sizeof(tx)) != HAL_OK)
    {
        s_stat[ch].spi_err_cnt++;
        return false;
    }

    /* rx[0] 은 opcode 송신 중 수신 바이트 (무시) */
    uint32_t frame = ((uint32_t)rx[1] << 24) | ((uint32_t)rx[2] << 16) |
                     ((uint32_t)rx[3] << 8)  |  (uint32_t)rx[4];
    uint32_t shift = 32U - MU150_POS_BITS;              /* 17bit → 15 */

    pos->frame   = frame;
    pos->pos_raw = frame >> shift;
    pos->err     = (((frame >> (shift - 1U)) & 1U) == 0U);   /* nERR (active low)  TODO(HW) 순서 확인 */
    pos->warn    = (((frame >> (shift - 2U)) & 1U) == 0U);   /* nWARN (active low) */

    s_stat[ch].read_cnt++;
    if (pos->err)
    {
        s_stat[ch].sensor_err_cnt++;
    }
    return true;
}

bool MU150_ReadRegister(MU150_Ch_e ch, uint8_t addr, uint8_t *data)
{
    BSP_SpiDev_e dev = MU_ToSpiDev(ch);
    uint8_t tx[3] = { MU150_OP_READ_REG, addr, 0U };
    uint8_t rx[3];

    /* 1) 읽기 요청 */
    if (BSP_SPI_Transfer(dev, tx, rx, 2U) != HAL_OK)
    {
        s_stat[ch].spi_err_cnt++;
        return false;
    }

    /* 2) busy 해제까지 상태 폴링 → [status][data] */
    tx[0] = MU150_OP_REG_STATUS;
    tx[1] = 0U;
    tx[2] = 0U;
    for (uint32_t i = 0U; i < MU150_REG_BUSY_RETRY; i++)
    {
        if (BSP_SPI_Transfer(dev, tx, rx, 3U) != HAL_OK)
        {
            s_stat[ch].spi_err_cnt++;
            return false;
        }
        if ((rx[1] & MU150_STATUS_BUSY_BIT) == 0U)
        {
            *data = rx[2];
            return true;
        }
    }
    s_stat[ch].timeout_cnt++;
    return false;
}

bool MU150_WriteRegister(MU150_Ch_e ch, uint8_t addr, uint8_t data)
{
    uint8_t tx[3] = { MU150_OP_WRITE_REG, addr, data };
    uint8_t rx[3];

    if (BSP_SPI_Transfer(MU_ToSpiDev(ch), tx, rx, 3U) != HAL_OK)
    {
        s_stat[ch].spi_err_cnt++;
        return false;
    }
    return true;
}

bool MU150_IsNoniusError(MU150_Ch_e ch, bool *nonius_err)
{
    uint8_t st;
    if (!MU150_ReadRegister(ch, MU150_REG_STATUS1, &st))
    {
        return false;
    }
    *nonius_err = ((st >> 3) & 1U) != 0U;
    return true;
}

const MU150_Stat_t *MU150_GetStat(MU150_Ch_e ch)
{
    return &s_stat[ch];
}
