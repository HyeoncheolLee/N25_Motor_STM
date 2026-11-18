/**
 * @file    dev_m95128.c
 */
#include "dev_m95128.h"
#include "board_config.h"
#include "bsp.h"
#include <string.h>

#define EE_CMD_WREN         (0x06U)
#define EE_CMD_RDSR         (0x05U)
#define EE_CMD_READ         (0x03U)
#define EE_CMD_WRITE        (0x02U)
#define EE_SR_WIP           (0x01U)
#define EE_SR_WEL           (0x02U)
#define EE_WRITE_TIMEOUT_MS (10U)
#define EE_CHUNK            (32U)       /* 한 번의 SPI 전송 데이터 크기 */

static bool EE_ReadStatus(uint8_t *sr)
{
    uint8_t tx[2] = { EE_CMD_RDSR, 0U };
    uint8_t rx[2] = { 0U };
    if (BSP_SPI_Transfer(BSP_SPI_DEV_EEPROM, tx, rx, 2U) != HAL_OK)
    {
        return false;
    }
    *sr = rx[1];
    return true;
}

static bool EE_WaitReady(void)
{
    uint32_t t0 = BSP_GetTickMs();
    uint8_t sr = EE_SR_WIP;

    do
    {
        if (!EE_ReadStatus(&sr))
        {
            return false;
        }
        if ((sr & EE_SR_WIP) == 0U)
        {
            return true;
        }
    } while ((BSP_GetTickMs() - t0) < EE_WRITE_TIMEOUT_MS);
    return false;
}

bool EEPROM_IsPresent(void)
{
    uint8_t sr = 0xFFU;
    /* 미장착/단선 시 MISO 가 0xFF 또는 0x00 으로 고정되는 경우가 많다 */
    return EE_ReadStatus(&sr) && (sr != 0xFFU);
}

bool EEPROM_Read(uint16_t addr, void *buf, uint16_t len)
{
    uint8_t  hdr[3] = { EE_CMD_READ, (uint8_t)(addr >> 8), (uint8_t)addr };
    uint8_t  dummy[3];
    uint8_t  zeros[EE_CHUNK];
    uint8_t *dst = (uint8_t *)buf;
    bool     ok;

    if (((uint32_t)addr + len) > EEPROM_SIZE_BYTES)
    {
        return false;
    }
    memset(zeros, 0, sizeof(zeros));

    if (BSP_SPI_Select(BSP_SPI_DEV_EEPROM) != HAL_OK)
    {
        return false;
    }
    ok = (BSP_SPI_TxRx(hdr, dummy, 3U) == HAL_OK);
    while (ok && (len > 0U))
    {
        uint16_t n = (len > EE_CHUNK) ? EE_CHUNK : len;
        ok   = (BSP_SPI_TxRx(zeros, dst, n) == HAL_OK);
        dst += n;
        len -= n;
    }
    BSP_SPI_Deselect(BSP_SPI_DEV_EEPROM);
    return ok;
}

/* 한 페이지 안쪽 쓰기 (len ≤ 페이지 남은 공간) */
static bool EE_WritePage(uint16_t addr, const uint8_t *src, uint16_t len)
{
    uint8_t wren = EE_CMD_WREN;
    uint8_t hdr[3] = { EE_CMD_WRITE, (uint8_t)(addr >> 8), (uint8_t)addr };
    uint8_t rxdummy[EE_CHUNK + 3U];
    bool ok;

    if (BSP_SPI_Transfer(BSP_SPI_DEV_EEPROM, &wren, rxdummy, 1U) != HAL_OK)
    {
        return false;
    }
    if (BSP_SPI_Select(BSP_SPI_DEV_EEPROM) != HAL_OK)
    {
        return false;
    }
    ok = (BSP_SPI_TxRx(hdr, rxdummy, 3U) == HAL_OK);
    while (ok && (len > 0U))
    {
        uint16_t n = (len > EE_CHUNK) ? EE_CHUNK : len;
        ok   = (BSP_SPI_TxRx(src, rxdummy, n) == HAL_OK);
        src += n;
        len -= n;
    }
    BSP_SPI_Deselect(BSP_SPI_DEV_EEPROM);     /* CS High → 내부 쓰기 시작 */
    return ok && EE_WaitReady();
}

bool EEPROM_Write(uint16_t addr, const void *buf, uint16_t len)
{
    const uint8_t *src = (const uint8_t *)buf;
    bool ok = true;

    if (((uint32_t)addr + len) > EEPROM_SIZE_BYTES)
    {
        return false;
    }

    BSP_EEPROM_WriteProtect(false);
    while (ok && (len > 0U))
    {
        uint16_t room = (uint16_t)(EEPROM_PAGE_BYTES - (addr % EEPROM_PAGE_BYTES));
        uint16_t n    = (len < room) ? len : room;
        ok    = EE_WritePage(addr, src, n);
        addr += n;
        src  += n;
        len  -= n;
    }
    BSP_EEPROM_WriteProtect(true);
    return ok;
}
