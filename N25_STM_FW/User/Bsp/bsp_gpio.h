/**
 ******************************************************************************
 * @file    bsp_gpio.h
 * @brief   보드 GPIO (LED, DRV8353 ENABLE/nFAULT, EEPROM WP)
 *
 *  핀 이름은 CubeMX "User Label" 로 지정 → main.h 에 XXX_Pin / XXX_GPIO_Port 생성.
 *  (docs/CubeMX_Setup.md 의 Label 표와 반드시 일치시킬 것)
 *
 *  SPI Chip Select 는 bsp_spi 에서 관리한다.
 ******************************************************************************
 */
#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    BSP_LED_A = 0,      /* PB7  (green) */
    BSP_LED_B,          /* PC14 (green) - backup domain pin: 저속/소전류만 */
    BSP_LED_NUM
} BSP_Led_e;

void BSP_GPIO_Init(void);

/* LED */
void BSP_LED_On(BSP_Led_e led);
void BSP_LED_Off(BSP_Led_e led);
void BSP_LED_Toggle(BSP_Led_e led);

/* DRV8353 gate driver */
void BSP_DRV_Enable(void);              /* ENABLE = High (PA3) */
void BSP_DRV_Disable(void);             /* ENABLE = Low  → 게이트 OFF, sleep */
bool BSP_DRV_IsEnabled(void);
bool BSP_DRV_IsFault(void);             /* nFAULT(PC13) == Low 이면 true */

/* EEPROM write-protect (PB8, BOOT0 겸용 핀 주의) */
void BSP_EEPROM_WriteProtect(bool protect);

#endif /* BSP_GPIO_H */
