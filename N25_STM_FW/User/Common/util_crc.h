/**
 * @file    util_crc.h
 * @brief   CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) — 파라미터 블록 무결성 검사용
 */
#ifndef UTIL_CRC_H
#define UTIL_CRC_H

#include <stdint.h>

uint16_t CRC16_Calc(const void *data, uint32_t len);

#endif /* UTIL_CRC_H */
