/**
 * @file    util_maf.c
 * @brief   Bit-shift Moving Average Filter
 */
#include "util_maf.h"

void MAF_Init(MAF_Handle_t *hmaf, int32_t *buf, uint8_t shift)
{
    if (shift == 0U)          { shift = 1U; }
    if (shift > MAF_SHIFT_MAX) { shift = MAF_SHIFT_MAX; }

    hmaf->buf   = buf;
    hmaf->shift = shift;
    hmaf->mask  = (uint16_t)((1U << shift) - 1U);
    MAF_Reset(hmaf, 0);
}

/* 버퍼 전체를 init_value 로 채운다 → 첫 출력부터 init_value (시작 램프 없음) */
void MAF_Reset(MAF_Handle_t *hmaf, int32_t init_value)
{
    uint16_t n = (uint16_t)(hmaf->mask + 1U);
    for (uint16_t i = 0U; i < n; i++)
    {
        hmaf->buf[i] = init_value;
    }
    hmaf->sum   = init_value * (int32_t)n;
    hmaf->idx   = 0U;
    hmaf->ready = 0U;
}
