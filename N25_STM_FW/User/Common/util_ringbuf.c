/**
 * @file    util_ringbuf.c
 */
#include "util_ringbuf.h"
#include <string.h>

void RingBuf_Init(RingBuf_Handle_t *hrb, void *buf, uint16_t item_size, uint16_t capacity_pow2)
{
    hrb->buf          = (uint8_t *)buf;
    hrb->item_size    = item_size;
    hrb->mask         = (uint16_t)(capacity_pow2 - 1U);
    hrb->head         = 0U;
    hrb->tail         = 0U;
    hrb->overflow_cnt = 0U;
}

bool RingBuf_Push(RingBuf_Handle_t *hrb, const void *item)
{
    uint16_t next = (uint16_t)((hrb->head + 1U) & hrb->mask);
    if (next == hrb->tail)
    {
        hrb->overflow_cnt++;
        return false;               /* full: 새 데이터 버림 */
    }
    memcpy(&hrb->buf[(uint32_t)hrb->head * hrb->item_size], item, hrb->item_size);
    hrb->head = next;
    return true;
}

bool RingBuf_Pop(RingBuf_Handle_t *hrb, void *item)
{
    if (hrb->tail == hrb->head)
    {
        return false;               /* empty */
    }
    memcpy(item, &hrb->buf[(uint32_t)hrb->tail * hrb->item_size], hrb->item_size);
    hrb->tail = (uint16_t)((hrb->tail + 1U) & hrb->mask);
    return true;
}

uint16_t RingBuf_Count(const RingBuf_Handle_t *hrb)
{
    return (uint16_t)((hrb->head - hrb->tail) & hrb->mask);
}
