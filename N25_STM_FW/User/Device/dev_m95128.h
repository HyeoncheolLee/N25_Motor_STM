/**
 ******************************************************************************
 * @file    dev_m95128.h
 * @brief   ST M95128 SPI EEPROM (128Kbit = 16KB, 64B page, 16bit address)
 *
 *   명령: WREN 0x06 / WRDI 0x04 / RDSR 0x05 / WRSR 0x01 / READ 0x03 / WRITE 0x02
 *   쓰기 사이클 최대 5ms → WIP 비트 폴링
 *   쓰기 중 /W(PB8) 를 High 로 풀어야 한다 (BSP_EEPROM_WriteProtect)
 ******************************************************************************
 */
#ifndef DEV_M95128_H
#define DEV_M95128_H

#include <stdint.h>
#include <stdbool.h>

bool EEPROM_Read(uint16_t addr, void *buf, uint16_t len);
bool EEPROM_Write(uint16_t addr, const void *buf, uint16_t len);   /* 페이지 경계 자동 분할 */
bool EEPROM_IsPresent(void);                                       /* RDSR 응답 확인 */

#endif /* DEV_M95128_H */
