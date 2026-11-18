/**
 * @file    util_crc.c
 */
#include "util_crc.h"

uint16_t CRC16_Calc(const void *data, uint32_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint16_t crc = 0xFFFFU;

    while (len--)
    {
        crc ^= (uint16_t)((uint16_t)(*p++) << 8);
        for (uint8_t i = 0U; i < 8U; i++)
        {
            crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}
