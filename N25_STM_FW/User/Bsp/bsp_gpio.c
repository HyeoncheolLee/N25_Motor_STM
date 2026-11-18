/**
 * @file    bsp_gpio.c
 */
#include "bsp_gpio.h"
#include "main.h"           /* CubeMX: XXX_Pin, XXX_GPIO_Port */

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t      pin;
} BSP_PinMap_t;

static const BSP_PinMap_t s_led_map[BSP_LED_NUM] =
{
    [BSP_LED_A] = { LED_A_GPIO_Port, LED_A_Pin },
    [BSP_LED_B] = { LED_B_GPIO_Port, LED_B_Pin },
};

void BSP_GPIO_Init(void)
{
    /* GPIO 모드/초기 레벨은 CubeMX MX_GPIO_Init()에서 설정됨.
     * 여기서는 "안전 상태"를 한 번 더 강제한다. */
    BSP_DRV_Disable();
    BSP_EEPROM_WriteProtect(true);
    BSP_LED_Off(BSP_LED_A);
    BSP_LED_Off(BSP_LED_B);
}

/* ---------------------------------------------------------------- LED --- */
/* LED: 회로상 Anode=+3.3V, Cathode=MCU → Low 에서 점등 (TODO(HW) 확인) */
void BSP_LED_On(BSP_Led_e led)
{
    HAL_GPIO_WritePin(s_led_map[led].port, s_led_map[led].pin, GPIO_PIN_RESET);
}

void BSP_LED_Off(BSP_Led_e led)
{
    HAL_GPIO_WritePin(s_led_map[led].port, s_led_map[led].pin, GPIO_PIN_SET);
}

void BSP_LED_Toggle(BSP_Led_e led)
{
    HAL_GPIO_TogglePin(s_led_map[led].port, s_led_map[led].pin);
}

/* ------------------------------------------------------------ DRV8353 --- */
void BSP_DRV_Enable(void)
{
    HAL_GPIO_WritePin(DRV_EN_GPIO_Port, DRV_EN_Pin, GPIO_PIN_SET);
}

void BSP_DRV_Disable(void)
{
    HAL_GPIO_WritePin(DRV_EN_GPIO_Port, DRV_EN_Pin, GPIO_PIN_RESET);
}

bool BSP_DRV_IsEnabled(void)
{
    return (HAL_GPIO_ReadPin(DRV_EN_GPIO_Port, DRV_EN_Pin) == GPIO_PIN_SET);
}

/* nFAULT 핀은 TIM1_BKIN(AF)로 설정되지만 IDR 읽기는 가능 */
bool BSP_DRV_IsFault(void)
{
    return (HAL_GPIO_ReadPin(DRV_NFAULT_GPIO_Port, DRV_NFAULT_Pin) == GPIO_PIN_RESET);
}

/* ------------------------------------------------------------- EEPROM --- */
/* M95128 /W: Low = 보호(쓰기 금지) */
void BSP_EEPROM_WriteProtect(bool protect)
{
    HAL_GPIO_WritePin(ROM_WP_GPIO_Port, ROM_WP_Pin, protect ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
