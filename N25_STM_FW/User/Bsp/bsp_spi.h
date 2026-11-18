/**
 ******************************************************************************
 * @file    bsp_spi.h
 * @brief   SPI1 공유 버스 관리 (DRV8353 / M95128 EEPROM / iC-MU150 #1 / #2)
 *
 *  하나의 SPI1 버스에 4개 디바이스가 붙어 있고, 디바이스마다 SPI 모드가 다르다.
 *      DRV8353 : Mode 1 (CPOL=0, CPHA=1), 16bit 프레임 → 8bit x2 로 전송
 *      M95128  : Mode 0 (Mode 3도 가능)
 *      iC-MU150: Mode 0 (Mode 3도 가능)
 *
 *  규칙 (중요):
 *   1. SPI 접근은 "main loop 컨텍스트"에서만 한다. ISR 에서 호출 금지.
 *      → 20kHz 제어 루프는 ABZ(타이머 엔코더)만 사용, SPI 절대값은 저속 감시용.
 *   2. 디바이스 선택 시 해당 모드로 자동 전환한다 (BSP_SPI_Select).
 *   3. 외부 디버그 커넥터(J1)로 MU150 을 설정할 때는 BSP_SPI_ReleaseBus()로
 *      MCU 쪽 SPI 핀을 High-Z 로 만들어 버스 충돌을 막는다.
 ******************************************************************************
 */
#ifndef BSP_SPI_H
#define BSP_SPI_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32g4xx_hal.h"

typedef enum
{
    BSP_SPI_DEV_DRV8353 = 0,
    BSP_SPI_DEV_EEPROM,
    BSP_SPI_DEV_MU1,
    BSP_SPI_DEV_MU2,
    BSP_SPI_DEV_NUM
} BSP_SpiDev_e;

void              BSP_SPI_Init(void);
HAL_StatusTypeDef BSP_SPI_Select(BSP_SpiDev_e dev);       /* 모드 전환 + CS Low */
void              BSP_SPI_Deselect(BSP_SpiDev_e dev);     /* CS High */
HAL_StatusTypeDef BSP_SPI_TxRx(const uint8_t *tx, uint8_t *rx, uint16_t len);

/* 한 번에: Select → TxRx → Deselect */
HAL_StatusTypeDef BSP_SPI_Transfer(BSP_SpiDev_e dev, const uint8_t *tx, uint8_t *rx, uint16_t len);

void              BSP_SPI_ReleaseBus(void);   /* 외부 툴 사용 시 */
void              BSP_SPI_AcquireBus(void);

uint32_t          BSP_SPI_GetErrorCount(void);

#endif /* BSP_SPI_H */
