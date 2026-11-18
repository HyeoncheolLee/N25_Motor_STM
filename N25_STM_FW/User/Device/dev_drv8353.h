/**
 ******************************************************************************
 * @file    dev_drv8353.h
 * @brief   TI DRV8353S 스마트 게이트드라이버 (SPI) 드라이버
 *
 *  SPI 프레임 (16bit, Mode 1):
 *    [15] R/W (1 = read) | [14:11] address | [10:0] data
 *
 *  N25 이슈 반영:
 *   - VDS 과전류(OCP) 임계, OCP 동작 모드를 FW 에서 명시적으로 설정/검증
 *   - Fault 레지스터를 주기적으로 읽어 원인(VDS_OCP / UVLO / GDF / OTSD ...)을 기록
 *
 *  !! 레지스터 비트 정의는 DRV8353 데이터시트(SLVSE65)로 최종 확인할 것 !!
 ******************************************************************************
 */
#ifndef DEV_DRV8353_H
#define DEV_DRV8353_H

#include <stdint.h>
#include <stdbool.h>

/* ---------------------------------------------------------- Registers --- */
#define DRV8353_REG_FAULT_STATUS1   (0x00U)
#define DRV8353_REG_VGS_STATUS2     (0x01U)
#define DRV8353_REG_DRIVER_CTRL     (0x02U)
#define DRV8353_REG_GATE_DRIVE_HS   (0x03U)
#define DRV8353_REG_GATE_DRIVE_LS   (0x04U)
#define DRV8353_REG_OCP_CTRL        (0x05U)
#define DRV8353_REG_CSA_CTRL        (0x06U)
#define DRV8353_REG_DRIVER_CFG      (0x07U)

/* Fault Status 1 (0x00) */
#define DRV8353_FS1_FAULT           (1U << 10)
#define DRV8353_FS1_VDS_OCP         (1U << 9)
#define DRV8353_FS1_GDF             (1U << 8)
#define DRV8353_FS1_UVLO            (1U << 7)
#define DRV8353_FS1_OTSD            (1U << 6)
#define DRV8353_FS1_VDS_HA          (1U << 5)
#define DRV8353_FS1_VDS_LA          (1U << 4)
#define DRV8353_FS1_VDS_HB          (1U << 3)
#define DRV8353_FS1_VDS_LB          (1U << 2)
#define DRV8353_FS1_VDS_HC          (1U << 1)
#define DRV8353_FS1_VDS_LC          (1U << 0)

/* VGS Status 2 (0x01) */
#define DRV8353_VS2_SA_OC           (1U << 10)
#define DRV8353_VS2_SB_OC           (1U << 9)
#define DRV8353_VS2_SC_OC           (1U << 8)
#define DRV8353_VS2_OTW             (1U << 7)
#define DRV8353_VS2_GDUV            (1U << 6)

/* Driver Control (0x02) */
#define DRV8353_DC_CLR_FLT          (1U << 0)
#define DRV8353_DC_PWM_MODE_6X      (0U << 5)

/* CSA gain 코드 (CSA Control [7:6]) */
typedef enum
{
    DRV8353_CSA_GAIN_5  = 0,
    DRV8353_CSA_GAIN_10 = 1,
    DRV8353_CSA_GAIN_20 = 2,
    DRV8353_CSA_GAIN_40 = 3
} DRV8353_CsaGain_e;

/* OCP 동작 모드 (OCP Control [7:6]) */
typedef enum
{
    DRV8353_OCP_LATCH       = 0,    /* 래치 셧다운 (권장: FW 가 원인 확인 후 해제) */
    DRV8353_OCP_AUTO_RETRY  = 1,
    DRV8353_OCP_REPORT_ONLY = 2,
    DRV8353_OCP_DISABLED    = 3
} DRV8353_OcpMode_e;

/* 설정 묶음 (파라미터에서 채움) */
typedef struct
{
    DRV8353_CsaGain_e csa_gain;
    DRV8353_OcpMode_e ocp_mode;
    uint8_t           vds_lvl;          /* 0~15 (0x4 ≈ 0.1V) → 차단전류 = VDS / Rds(on) */
    uint8_t           ocp_deglitch;     /* 0~3 */
    uint8_t           dead_time;        /* 0:50ns 1:100ns 2:200ns 3:400ns */
    uint8_t           idrivep_hs;       /* 0~15 source current code */
    uint8_t           idriven_hs;       /* 0~15 sink   current code */
    uint8_t           idrivep_ls;
    uint8_t           idriven_ls;
    uint8_t           sen_lvl;          /* 0~3 sense OCP level */
} DRV8353_Config_t;

typedef struct
{
    uint16_t fault1;
    uint16_t vgs2;
    uint32_t spi_err_cnt;
    uint32_t verify_err_cnt;
    bool     initialized;
} DRV8353_Status_t;

void     DRV8353_GetDefaultConfig(DRV8353_Config_t *cfg);
bool     DRV8353_Init(const DRV8353_Config_t *cfg);   /* ENABLE → 레지스터 기록 → read-back 검증 */
bool     DRV8353_ReadReg(uint8_t addr, uint16_t *val);
bool     DRV8353_WriteReg(uint8_t addr, uint16_t val);
bool     DRV8353_ReadFaults(uint16_t *fault1, uint16_t *vgs2);
bool     DRV8353_ClearFaults(void);
const DRV8353_Status_t *DRV8353_GetStatus(void);

#endif /* DEV_DRV8353_H */
