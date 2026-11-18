# N25 STM32G474 — CubeMX(.ioc) 설정 가이드

> 대상 MCU: **STM32G474CETx (LQFP48)**
> 이 문서대로 CubeMX를 설정해야 `User/` 코드의 핀 Label, 핸들 이름(`htim1`, `hadc1` 등)이 맞는다.
> 표시 **[확인]** = CubeMX에서 해당 AF/채널이 제안되는지 꼭 확인할 항목 (데이터시트 기준 추정값).

---

## 0. 프로젝트 생성

1. STM32CubeIDE → File → New → STM32 Project → `STM32G474CETx`
2. Project name: `N25_STM_FW`, Targeted Language: C, Binary: Executable
3. **Project Manager → Code Generator**
   - [x] **Generate peripheral initialization as a pair of '.c/.h' files per peripheral** ← 필수 (`tim.h`, `adc.h` 등 생성)
   - [x] Keep User Code when re-generating
   - [x] Delete previously generated files when not re-generated
4. **Pinout & Configuration → System Core → SYS**
   - Debug: **Serial Wire**
   - Timebase Source: **SysTick** ← 필수 (`HAL_TIM_PeriodElapsedCallback` 중복 방지)

---

## 1. Clock (RCC)

| 항목 | 값 |
|------|-----|
| HSE | **BYPASS Clock Source** (Y1은 발진기 모듈 → PF0 입력) |
| LSE | Disable (PC14 = LED_B로 사용) |
| PLL Source | HSE 24MHz |
| PLLM / PLLN / PLLR | /6, ×85, /2 → **SYSCLK 170MHz** |
| PLLQ | /2 (미사용 가능) |
| AHB / APB1 / APB2 | /1, /1, /1 → 170MHz |
| Power | Range 1 **Boost** (170MHz 필요, CubeMX 자동) |
| ADC12 / ADC345 clock | **Synchronous HCLK/4** (42.5MHz) |
| FDCAN clock | **PCLK1** (170MHz) |

---

## 2. GPIO 및 User Label  (Label 이름 = 코드의 `XXX_Pin`, `XXX_GPIO_Port`)

| 핀 | Label | 모드 | 초기값 / 설정 | 회로 신호 |
|----|-------|------|--------------|-----------|
| PA3 | **DRV_EN** | GPIO_Output PP | **Low**, Pull-down | DRV8353 ENABLE |
| PC13 | **DRV_NFAULT** | **TIM1_BKIN** (AF) [확인] | 외부 pull-up | DRV8353 nFAULT |
| PB0 | **SPI_CS_DRV** | GPIO_Output PP | **High** | SPI1_CS_DRV |
| PA5 | **SPI_CS_MU1** | GPIO_Output PP | **High** | SPI1_CS_MU (U6) |
| PB10 | **SPI_CS_MU2** | GPIO_Output PP | **High** | SPI2_CS_MU (U10) |
| PB6 | **SPI_CS_ROM** | GPIO_Output PP | **High** | SPI1_CS_ROM |
| PB8 | **ROM_WP** | GPIO_Output PP | **Low** (쓰기금지) | EEPROM /W (BOOT0 겸용!) |
| PB7 | **LED_A** | GPIO_Output PP | High (OFF) | LED_A |
| PC14 | **LED_B** | GPIO_Output PP | High (OFF), Speed Low | LED_B |
| PB3 | **SPI1_SCK** | SPI1_SCK | | SPI1_CLK |
| PB4 | **SPI1_MISO** | SPI1_MISO | | SPI1_MISO |
| PB5 | **SPI1_MOSI** | SPI1_MOSI | | SPI1_MOSI |
| PA8 / PB13 | — | TIM1_CH1 / CH1N | | PWM_H1 / PWM_L1 |
| PA9 / PB14 | — | TIM1_CH2 / CH2N | | PWM_H2 / PWM_L2 |
| PA10 / PB15 | — | TIM1_CH3 / CH3N | | PWM_H3 / PWM_L3 |
| PA0 / PA1 | ENC1_A / ENC1_B | TIM2_CH1 / CH2 | | ENC_A / ENC_B |
| PA15 | ENC1_Z | GPIO_Input (추후 TIM2_ETR index) [확인] | | ENC_I |
| PA6 / PA7 | ENC2_A / ENC2_B | TIM3_CH1 / CH2 | | ENC2_A / ENC2_B |
| PB1 | CUR_U | **ADC3_IN1** [확인] | | U_CUR |
| PB12 | CUR_V | **ADC4_IN3** [확인] | | V_CUR |
| PA2 | CUR_W | **ADC1_IN3** | | W_CUR |
| PA4 | VBUS_SENSE | **ADC2_IN17** [확인] | | V_BAT |
| PB2 | TEMP_BOARD | **ADC2_IN12** | | TEMP_BOARD |
| PB11 | TEMP_MOTOR | **ADC2_IN14** | | TEMP_MOTOR |
| PB9 | BRAKE_PWM | **TIM17_CH1** [확인] | | SolGnd |
| PA11 / PA12 | — | FDCAN1_RX / TX | | CANRX / CANTX |
| PA13 / PA14 | — | SWDIO / SWCLK | | |
| PF0 | — | RCC_OSC_IN | | 24MHz_OSC |

> 전류 채널은 **ADC1/3/4 세 개를 동시에** 쓴다 (W는 ADC1에만 연결 가능한 핀이라 이렇게 나뉨).
> CubeMX가 다른 ADC 번호를 제안하면 `bsp_adc.c`의 핸들/채널만 바꾸면 된다.

---

## 3. TIM1 — 3상 PWM

| 항목 | 값 |
|------|-----|
| Clock Source | Internal |
| Channel1~3 | **PWM Generation CHx CHxN** |
| Channel4 | **PWM Generation No Output** |
| Prescaler | 0 |
| Counter Mode | **Center Aligned mode 1** |
| Period (ARR) | **4250** (170MHz / (2×20kHz)) |
| Repetition Counter | 1 |
| auto-reload preload | Enable |
| **TRGO2** (Trigger Event Selection 2) | **OC4REF rising edge** (또는 `OC4REF rising or falling` 중 rising) |
| CH1~3 Mode / Pulse | PWM mode 1 / 2125 |
| CH1~3 Polarity / N Polarity | High / High |
| CH1~3 Idle / N Idle state | Reset / Reset |
| CH1~3 Output compare preload | Enable |
| CH4 Mode / Pulse | **PWM mode 2 / 4249** (ARR−1 → 카운터 peak = low-side ON 중앙에서 ADC 트리거) |
| Dead Time | **51** (≈300ns @5.88ns) |
| BRK (Break) | **Enable**, Polarity **Low**, Filter 4, AF mode = Input |
| BRK2 | Disable |
| Automatic Output | **Disable** (Fault 후 SW 확인 전까지 출력 재개 금지) |
| Off State Run / Idle | Enable / Enable |
| Lock | Off |
| NVIC | **TIM1 break interrupt and TIM15 global interrupt: Enable** |

---

## 4. ADC

### ADC1 (W 전류) / ADC3 (U 전류) / ADC4 (V 전류) — 공통
| 항목 | 값 |
|------|-----|
| Mode | Independent |
| Resolution | 12bit, Right align |
| Regular conversion | 사용 안 함 (CubeMX가 요구하면 1채널 + Software trigger 로 두고 시작하지 않음) |
| **Injected Conversion** | Enable, Number of conversions = 1 |
| Injected Channel | ADC1: IN3 / ADC3: IN1 / ADC4: IN3, Single-ended |
| Sampling Time | **6.5 Cycles** (CSA 출력 + 56Ω/4.7nF RC) |
| External Trigger Source | **Timer 1 Trigger Out 2 event** |
| External Trigger Edge | Rising |
| Injected Auto / Queue | Disable |
| NVIC | **ADC1 and ADC2 global interrupt: Enable** (ADC3/ADC4 인터럽트는 Disable) |

### ADC2 (저속: Vbus, 온도)
| 항목 | 값 |
|------|-----|
| Scan Conversion Mode | Enable |
| Continuous Conversion | **Enable** |
| DMA Continuous Requests | **Enable** |
| Overrun | Overwritten |
| Number of Conversion | 3 |
| Rank1 / 2 / 3 | **IN17 (VBUS)** / **IN12 (TEMP_BOARD)** / **IN14 (TEMP_MOTOR)** |
| Sampling Time | **247.5 Cycles** (Vbus 분압 출력저항 ≈17kΩ) |
| Trigger | Software |
| DMA | ADC2 → DMA1 Channel x, **Circular**, Half Word / Half Word, Memory increment |

---

## 5. TIM2 / TIM3 — 엔코더

| 항목 | TIM2 (ENC1) | TIM3 (ENC2) |
|------|-------------|-------------|
| Combined Channels | Encoder Mode | Encoder Mode |
| Encoder Mode | TI1 and TI2 (x4) | TI1 and TI2 (x4) |
| Period | **4294967295** (32bit) | **65535** |
| Input Filter | 4 | 4 |
| Polarity | Rising (방향이 반대면 한쪽 Falling) | Rising |

---

## 6. TIM6 — 1kHz 스케줄러 / TIM17 — 브레이크 PWM

| 항목 | TIM6 | TIM17 |
|------|------|-------|
| 용도 | 1ms tick | 솔레노이드 20kHz PWM |
| Prescaler | 169 | 0 |
| Period | 999 | 8499 |
| Mode | Activated | CH1 PWM Generation, PWM mode 1, Pulse 0 |
| NVIC | **TIM6 global interrupt: Enable** | 불필요 |

---

## 7. SPI1

| 항목 | 값 |
|------|-----|
| Mode | Full-Duplex Master, Hardware NSS **Disable** |
| Data Size | 8 Bits, MSB First |
| Prescaler | **128** (1.33MHz, bring-up 시작값) → 검증 후 32 (5.3MHz) |
| CPOL / CPHA | Low / 1 Edge (Mode 0 기본, DRV 접근 시 코드가 Mode 1로 전환) |
| NSSP / CRC | Disable / Disable |

---

## 8. FDCAN1 — 1Mbps Classic CAN

| 항목 | 값 |
|------|-----|
| Frame Format | Classic mode |
| Mode | Normal mode |
| Auto Retransmission | Enable |
| Nominal Prescaler | **10** (170MHz → 17MHz) |
| Nominal Sync Jump Width | 3 |
| Nominal Time Seg1 / Seg2 | **13 / 3** → 1+13+3 = 17tq → 1Mbps, 샘플점 82% |
| Std Filters Nbr | **1** 이상 |
| Ext Filters Nbr | 0 |
| Tx Fifo Queue Mode | FIFO |
| NVIC | **FDCAN1 interrupt 0: Enable** |

---

## 9. IWDG

| 항목 | 값 |
|------|-----|
| Prescaler | 32 (LSI 32kHz → 1kHz) |
| Reload | **250** (≈250ms) |
| Window | 4095 (window off) |

---

## 10. NVIC 우선순위 (Preemption, 낮을수록 높음)

| 인터럽트 | 우선순위 | 이유 |
|----------|---------|------|
| TIM1 BRK | **0** | 게이트드라이버 Fault — HW가 이미 출력을 껐지만 즉시 기록 |
| ADC1_2 | **1** | 20kHz Fast Loop (보호 + 제어) |
| SysTick | 2 | HAL tick |
| FDCAN1 IT0 | 4 | 수신 → 링버퍼 |
| TIM6 | 5 | 1ms 플래그 |
| DMA1 (ADC2) | 6 | |

---

## 11. 코드 통합

1. 저장소의 `User/` 폴더를 CubeIDE 프로젝트 루트에 복사
2. 프로젝트 우클릭 → Properties → **C/C++ General → Paths and Symbols**
   - **Source Location**: Add Folder → `User`
   - **Includes (GNU C)**: `User/Config`, `User/Common`, `User/Bsp`, `User/Device`, `User/Service`, `User/App` 추가
3. `Core/Src/main.c` USER CODE 영역에 추가:
```c
/* USER CODE BEGIN Includes */
#include "app_main.h"
/* USER CODE END Includes */

  /* USER CODE BEGIN 2 */
  App_Init();
  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
    App_Loop();
  }
  /* USER CODE END 3 */
```
4. 링크 옵션에 수학 라이브러리 확인 (`-lm`, CubeIDE 기본 포함)
5. FPU: Project → Properties → MCU Settings → Floating-point unit `FPv4-SP-D16`, ABI `hard`

---

## 12. Option Bytes (첫 플래싱 직후 필수) ⚠

PB8(BOOT0)에 EEPROM WP의 4.7kΩ 풀업과 10kΩ 풀다운이 함께 연결되어 있다. 그래서 리셋 시 BOOT0 ≈ 2.2V로 레벨이 애매하다 ([HW 리뷰 H3](../../Study/06_N25_STM_HW_리뷰.md)).
**STM32CubeProgrammer → OB → User Configuration**:
- `nSWBOOT0` = **unchecked (0)** → BOOT0 핀을 무시하고 옵션 비트 사용
- `nBOOT0` = **checked (1)** → 항상 Main Flash에서 부팅
