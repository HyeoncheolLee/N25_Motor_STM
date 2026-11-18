/**
 ******************************************************************************
 * @file    bsp_encoder.h
 * @brief   ABZ 인크리멘탈 엔코더 카운터 (iC-MU150 의 ABZ 출력)
 *
 *   ENC1 (모터축 MU150 #1, U6) : TIM2 (32bit), A=PA0, B=PA1, Z=PA15(TIM2_ETR, index)
 *   ENC2 (보조   MU150 #2, U10): TIM3 (16bit), A=PA6, B=PA7   → SW 로 32bit 확장
 *
 *   20kHz Fast Loop 에서 BSP_ENC_Update() 호출 → 누적 카운트(int32) 갱신
 *   절대 위치(SPI)는 dev_mu150 에서 저속으로 읽어 BSP_ENC_SetCount()로 맞춘다.
 ******************************************************************************
 */
#ifndef BSP_ENCODER_H
#define BSP_ENCODER_H

#include <stdint.h>

typedef enum
{
    BSP_ENC_1 = 0,
    BSP_ENC_2,
    BSP_ENC_NUM
} BSP_Enc_e;

void    BSP_ENC_Init(void);
void    BSP_ENC_Update(void);                         /* Fast Loop 에서 호출 */
int32_t BSP_ENC_GetCount(BSP_Enc_e enc);              /* 누적 카운트 */
int32_t BSP_ENC_GetDelta(BSP_Enc_e enc);              /* 마지막 Update 간 변화량 */
void    BSP_ENC_SetCount(BSP_Enc_e enc, int32_t cnt); /* 절대값으로 재설정 */

#endif /* BSP_ENCODER_H */
