/**
 * @file    bsp_spi.c
 * @brief   SPI1 공유 버스: 디바이스별 CS + SPI 모드 전환
 */
#include "bsp_spi.h"
#include "main.h"
#include "spi.h"        /* CubeMX: extern SPI_HandleTypeDef hspi1; */

#define SPI_TIMEOUT_MS      (5U)

typedef struct
{
    GPIO_TypeDef *cs_port;
    uint16_t      cs_pin;
    uint32_t      cpol;         /* SPI_POLARITY_LOW / HIGH */
    uint32_t      cpha;         /* SPI_PHASE_1EDGE / 2EDGE */
} BSP_SpiDevCfg_t;

static const BSP_SpiDevCfg_t s_dev_cfg[BSP_SPI_DEV_NUM] =
{
    [BSP_SPI_DEV_DRV8353] = { SPI_CS_DRV_GPIO_Port, SPI_CS_DRV_Pin, SPI_POLARITY_LOW, SPI_PHASE_2EDGE },  /* Mode 1 */
    [BSP_SPI_DEV_EEPROM]  = { SPI_CS_ROM_GPIO_Port, SPI_CS_ROM_Pin, SPI_POLARITY_LOW, SPI_PHASE_1EDGE },  /* Mode 0 */
    [BSP_SPI_DEV_MU1]     = { SPI_CS_MU1_GPIO_Port, SPI_CS_MU1_Pin, SPI_POLARITY_LOW, SPI_PHASE_1EDGE },  /* Mode 0 */
    [BSP_SPI_DEV_MU2]     = { SPI_CS_MU2_GPIO_Port, SPI_CS_MU2_Pin, SPI_POLARITY_LOW, SPI_PHASE_1EDGE },  /* Mode 0 */
};

static int8_t   s_cur_mode_dev = -1;   /* 마지막으로 설정한 모드의 디바이스 */
static bool     s_bus_busy     = false;
static uint32_t s_err_cnt      = 0U;

static void SPI_AllCsHigh(void)
{
    for (uint32_t i = 0U; i < BSP_SPI_DEV_NUM; i++)
    {
        HAL_GPIO_WritePin(s_dev_cfg[i].cs_port, s_dev_cfg[i].cs_pin, GPIO_PIN_SET);
    }
}

/* CPOL/CPHA 만 바꾼다. 레지스터를 직접 바꾸고 hspi1.Init 도 동기화 (HAL 상태 일치) */
static void SPI_SetMode(uint32_t cpol, uint32_t cpha)
{
    if ((hspi1.Init.CLKPolarity == cpol) && (hspi1.Init.CLKPhase == cpha))
    {
        return;
    }
    __HAL_SPI_DISABLE(&hspi1);
    MODIFY_REG(hspi1.Instance->CR1, SPI_CR1_CPOL | SPI_CR1_CPHA, cpol | cpha);
    hspi1.Init.CLKPolarity = cpol;
    hspi1.Init.CLKPhase    = cpha;
    /* HAL_SPI_TransmitReceive 가 SPE 를 다시 켠다 */
}

void BSP_SPI_Init(void)
{
    SPI_AllCsHigh();
    s_cur_mode_dev = -1;
    s_bus_busy     = false;
}

HAL_StatusTypeDef BSP_SPI_Select(BSP_SpiDev_e dev)
{
    if ((dev >= BSP_SPI_DEV_NUM) || s_bus_busy)
    {
        s_err_cnt++;
        return HAL_BUSY;            /* 재진입(ISR 호출 등) 감지 */
    }
    s_bus_busy = true;

    if (s_cur_mode_dev != (int8_t)dev)
    {
        SPI_SetMode(s_dev_cfg[dev].cpol, s_dev_cfg[dev].cpha);
        s_cur_mode_dev = (int8_t)dev;
    }
    HAL_GPIO_WritePin(s_dev_cfg[dev].cs_port, s_dev_cfg[dev].cs_pin, GPIO_PIN_RESET);
    return HAL_OK;
}

void BSP_SPI_Deselect(BSP_SpiDev_e dev)
{
    if (dev < BSP_SPI_DEV_NUM)
    {
        HAL_GPIO_WritePin(s_dev_cfg[dev].cs_port, s_dev_cfg[dev].cs_pin, GPIO_PIN_SET);
    }
    s_bus_busy = false;
}

HAL_StatusTypeDef BSP_SPI_TxRx(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)tx, rx, len, SPI_TIMEOUT_MS);
    if (st != HAL_OK)
    {
        s_err_cnt++;
    }
    return st;
}

HAL_StatusTypeDef BSP_SPI_Transfer(BSP_SpiDev_e dev, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    HAL_StatusTypeDef st = BSP_SPI_Select(dev);
    if (st != HAL_OK)
    {
        return st;
    }
    st = BSP_SPI_TxRx(tx, rx, len);
    BSP_SPI_Deselect(dev);
    return st;
}

/* 외부 디버거(J1)가 MU150 을 직접 설정할 때: MCU SPI 핀을 입력(High-Z)으로 */
void BSP_SPI_ReleaseBus(void)
{
    GPIO_InitTypeDef gi = {0};

    SPI_AllCsHigh();
    __HAL_SPI_DISABLE(&hspi1);

    gi.Pin  = SPI1_SCK_Pin | SPI1_MISO_Pin | SPI1_MOSI_Pin;
    gi.Mode = GPIO_MODE_INPUT;
    gi.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SPI1_SCK_GPIO_Port, &gi);

    gi.Pin = SPI_CS_MU1_Pin;
    HAL_GPIO_Init(SPI_CS_MU1_GPIO_Port, &gi);
    gi.Pin = SPI_CS_MU2_Pin;
    HAL_GPIO_Init(SPI_CS_MU2_GPIO_Port, &gi);

    s_bus_busy = true;      /* 이후 MCU 의 SPI 접근 차단 */
}

void BSP_SPI_AcquireBus(void)
{
    GPIO_InitTypeDef gi = {0};

    /* SPI 핀(AF) 복원: CubeMX 생성 MSP 함수 재사용 */
    HAL_SPI_MspInit(&hspi1);

    /* CS 핀만 출력으로 복원 (MX_GPIO_Init 전체 재호출은 DRV_EN 등 다른 핀까지 초기화하므로 금지) */
    HAL_GPIO_WritePin(SPI_CS_MU1_GPIO_Port, SPI_CS_MU1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SPI_CS_MU2_GPIO_Port, SPI_CS_MU2_Pin, GPIO_PIN_SET);
    gi.Mode  = GPIO_MODE_OUTPUT_PP;
    gi.Pull  = GPIO_NOPULL;
    gi.Speed = GPIO_SPEED_FREQ_MEDIUM;
    gi.Pin   = SPI_CS_MU1_Pin;
    HAL_GPIO_Init(SPI_CS_MU1_GPIO_Port, &gi);
    gi.Pin   = SPI_CS_MU2_Pin;
    HAL_GPIO_Init(SPI_CS_MU2_GPIO_Port, &gi);

    SPI_AllCsHigh();
    s_cur_mode_dev = -1;
    s_bus_busy     = false;
}

uint32_t BSP_SPI_GetErrorCount(void)
{
    return s_err_cnt;
}
