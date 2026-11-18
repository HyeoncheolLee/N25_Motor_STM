/**
 * @file    bsp_encoder.c
 *
 *  누적 방식: 하드웨어 카운터의 "차이"를 부호있는 정수로 계산해 더한다.
 *    delta = (int16_t)(now - prev)   ← 16bit 타이머 wrap-around 자동 처리
 *  20kHz 에서 한 주기 변화량은 수 카운트 수준이므로 16bit 로 충분하다.
 */
#include "bsp_encoder.h"
#include "tim.h"            /* CubeMX: htim2, htim3 */

typedef struct
{
    int32_t  count;         /* 누적 카운트 */
    int32_t  delta;
    uint32_t hw_prev;
} BSP_EncState_t;

static BSP_EncState_t s_enc[BSP_ENC_NUM];

void BSP_ENC_Init(void)
{
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);

    s_enc[BSP_ENC_1].hw_prev = htim2.Instance->CNT;
    s_enc[BSP_ENC_2].hw_prev = htim3.Instance->CNT;
    s_enc[BSP_ENC_1].count = 0;
    s_enc[BSP_ENC_2].count = 0;
}

void BSP_ENC_Update(void)
{
    uint32_t now1 = htim2.Instance->CNT;                 /* 32bit */
    uint32_t now2 = htim3.Instance->CNT & 0xFFFFU;       /* 16bit */

    s_enc[BSP_ENC_1].delta = (int32_t)(now1 - s_enc[BSP_ENC_1].hw_prev);
    s_enc[BSP_ENC_2].delta = (int32_t)(int16_t)(uint16_t)(now2 - s_enc[BSP_ENC_2].hw_prev);

    s_enc[BSP_ENC_1].hw_prev = now1;
    s_enc[BSP_ENC_2].hw_prev = now2;

    s_enc[BSP_ENC_1].count += s_enc[BSP_ENC_1].delta;
    s_enc[BSP_ENC_2].count += s_enc[BSP_ENC_2].delta;
}

int32_t BSP_ENC_GetCount(BSP_Enc_e enc)
{
    return s_enc[enc].count;
}

int32_t BSP_ENC_GetDelta(BSP_Enc_e enc)
{
    return s_enc[enc].delta;
}

/* main loop 에서 호출 시 Fast Loop 와 경합 → 짧게 IRQ 차단 */
void BSP_ENC_SetCount(BSP_Enc_e enc, int32_t cnt)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    s_enc[enc].count = cnt;
    __set_PRIMASK(primask);
}
