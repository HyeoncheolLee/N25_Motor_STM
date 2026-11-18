/**
 ******************************************************************************
 * @file    dev_mu150.h
 * @brief   iC-MU150 자기식 Nonius 절대 엔코더 (SPI) 드라이버 — 2채널
 *
 *  Opcode (N25 FW 와 동일):
 *    0xA6  SDAD transmission (센서 위치 데이터)
 *    0xF5  SDAD status
 *    0x97  Read REGISTER (single)
 *    0xD2  Write REGISTER (single)
 *    0xAD  REGISTER status/data
 *    0xB0  Activate
 *
 *  위치 데이터 형식 (N25 기준: single-turn 17bit):
 *    [위치 17bit][nERR][nWARN][CRC6 ...]   ← MU150 설정(EEPROM)에 따라 달라짐
 *    TODO(HW): MU150 의 ST 분해능 / CRC 설정을 확인하고 MU150_POS_BITS 맞출 것
 *
 *  사용 위치:
 *    - 부팅 시: 절대각 → 전기각 원점 / ABZ 카운터 초기값
 *    - 운전 중(10~100Hz): ABZ 누적값과 비교 → 카운트 유실 감시 (N25 이슈 #4 대응)
 *    - 20kHz 제어 루프에서는 사용하지 않음 (SPI 는 main loop 전용)
 ******************************************************************************
 */
#ifndef DEV_MU150_H
#define DEV_MU150_H

#include <stdint.h>
#include <stdbool.h>

#define MU150_OP_ACTIVATE       (0xB0U)
#define MU150_OP_SDAD           (0xA6U)
#define MU150_OP_SDAD_STATUS    (0xF5U)
#define MU150_OP_READ_REG       (0x97U)
#define MU150_OP_WRITE_REG      (0xD2U)
#define MU150_OP_REG_STATUS     (0xADU)

#define MU150_REG_STATUS0       (0x76U)
#define MU150_REG_STATUS1       (0x77U)     /* N25: bit3 = Nonius 에러(NON_CTR) */

#define MU150_POS_BITS          (17U)       /* TODO(HW) */

typedef enum
{
    MU150_CH1 = 0,      /* U6  - 모터축 */
    MU150_CH2,          /* U10 - 보조 */
    MU150_CH_NUM
} MU150_Ch_e;

typedef struct
{
    uint32_t pos_raw;       /* MU150_POS_BITS 비트 절대 위치 */
    bool     err;           /* nERR = 0 → true */
    bool     warn;          /* nWARN = 0 → true */
    uint32_t frame;         /* 원시 32bit 프레임 (디버그) */
} MU150_Position_t;

typedef struct
{
    uint32_t read_cnt;
    uint32_t spi_err_cnt;
    uint32_t sensor_err_cnt;
    uint32_t timeout_cnt;
} MU150_Stat_t;

bool MU150_ReadPosition(MU150_Ch_e ch, MU150_Position_t *pos);
bool MU150_ReadRegister(MU150_Ch_e ch, uint8_t addr, uint8_t *data);
bool MU150_WriteRegister(MU150_Ch_e ch, uint8_t addr, uint8_t data);
bool MU150_IsNoniusError(MU150_Ch_e ch, bool *nonius_err);
const MU150_Stat_t *MU150_GetStat(MU150_Ch_e ch);

#endif /* DEV_MU150_H */
