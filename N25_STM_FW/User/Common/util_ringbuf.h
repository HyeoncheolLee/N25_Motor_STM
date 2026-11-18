/**
 * @file    util_ringbuf.h
 * @brief   고정 크기 레코드 링버퍼 (단일 생산자 ISR / 단일 소비자 main loop)
 *
 *  - 크기는 2의 거듭제곱 (인덱스 마스킹)
 *  - ISR(push) ↔ main(pop) 간 lock-free. head/tail 은 volatile.
 */
#ifndef UTIL_RINGBUF_H
#define UTIL_RINGBUF_H

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint8_t           *buf;         /* item_size * capacity 바이트 */
    uint16_t           item_size;
    uint16_t           mask;        /* capacity - 1 */
    volatile uint16_t  head;        /* write index (producer) */
    volatile uint16_t  tail;        /* read index  (consumer) */
    volatile uint32_t  overflow_cnt;
} RingBuf_Handle_t;

void     RingBuf_Init(RingBuf_Handle_t *hrb, void *buf, uint16_t item_size, uint16_t capacity_pow2);
bool     RingBuf_Push(RingBuf_Handle_t *hrb, const void *item);
bool     RingBuf_Pop(RingBuf_Handle_t *hrb, void *item);
uint16_t RingBuf_Count(const RingBuf_Handle_t *hrb);

#endif /* UTIL_RINGBUF_H */
