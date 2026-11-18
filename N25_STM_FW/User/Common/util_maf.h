/**
 ******************************************************************************
 * @file    util_maf.h
 * @brief   Bit-shift Moving Average Filter (MAF)
 *
 *  - 윈도우 크기 N = 2^shift  → 나눗셈 대신 오른쪽 시프트(>> shift) 사용
 *  - 누적합(running sum) 방식: 매 샘플 O(1) 연산 (덧셈 1, 뺄셈 1, 시프트 1)
 *
 *      sum  = sum + x[n] - x[n-N]
 *      y[n] = sum >> shift
 *
 *  - 군지연(group delay) = (N-1)/2 샘플.  예) 1kHz, N=16 → 7.5ms 지연
 *    → 제어 루프 피드백(전류 PI 등)에는 사용하지 않는다. 모니터링/보호/표시용.
 *  - 음수 입력: ARM GCC의 signed >> 는 산술 시프트(내림 방향)라 -0.5 LSB 편향이 있다.
 *    필요하면 MAF_GetRounded()를 사용한다.
 *
 *  사용 예)
 *      static int32_t vbus_buf[16];
 *      static MAF_Handle_t vbus_maf;
 *      MAF_Init(&vbus_maf, vbus_buf, 4);           // N = 2^4 = 16
 *      MAF_Reset(&vbus_maf, first_sample);          // 시작 램프 방지
 *      int32_t avg = MAF_Update(&vbus_maf, raw);
 ******************************************************************************
 */
#ifndef UTIL_MAF_H
#define UTIL_MAF_H

#include <stdint.h>

#define MAF_SHIFT_MAX   (10U)       /* 최대 N = 1024 */

typedef struct
{
    int32_t  *buf;      /* 길이 (1 << shift) 인 외부 버퍼 */
    int32_t   sum;      /* 누적합 (N * 입력최대 < 2^31 이 되도록 사용) */
    uint16_t  idx;      /* 다음에 덮어쓸 위치 */
    uint16_t  mask;     /* N - 1 (인덱스 wrap 용) */
    uint8_t   shift;    /* log2(N) */
    uint8_t   ready;    /* 버퍼가 한 바퀴 채워졌으면 1 */
} MAF_Handle_t;

void    MAF_Init(MAF_Handle_t *hmaf, int32_t *buf, uint8_t shift);
void    MAF_Reset(MAF_Handle_t *hmaf, int32_t init_value);

/* ISR에서도 쓰므로 inline */
static inline int32_t MAF_Update(MAF_Handle_t *hmaf, int32_t x)
{
    hmaf->sum += x - hmaf->buf[hmaf->idx];
    hmaf->buf[hmaf->idx] = x;
    hmaf->idx = (uint16_t)((hmaf->idx + 1U) & hmaf->mask);
    if (hmaf->idx == 0U) { hmaf->ready = 1U; }
    return (hmaf->sum >> hmaf->shift);
}

static inline int32_t MAF_Get(const MAF_Handle_t *hmaf)
{
    return (hmaf->sum >> hmaf->shift);
}

/* 0 방향 반올림 버전 (음수 편향 제거) */
static inline int32_t MAF_GetRounded(const MAF_Handle_t *hmaf)
{
    int32_t half = (int32_t)(1L << (hmaf->shift - 1U));
    return (hmaf->sum >= 0) ? ((hmaf->sum + half) >> hmaf->shift)
                            : -((-hmaf->sum + half) >> hmaf->shift);
}

#endif /* UTIL_MAF_H */
