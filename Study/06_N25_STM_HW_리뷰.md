# 06. N25_STM 회로도 리뷰 (STM32G474 신규 보드)

> 원본: `N25/N25_STM_Schematic PDF.pdf` (2 sheet: 전력/DRV/CAN/전원, MCU/엔코더/EEPROM/온도/디버그)
> PDF 텍스트와 넷리스트로 확인한 결과다. **[확인]** = 데이터시트나 실물로 최종 확인이 필요한 항목이다.

---

## 1. 구성 요약 (N25 PIC32 보드와 비교)

| 항목 | N25 (PIC32) | **N25_STM** | 평가 |
|------|-------------|-------------|------|
| MCU | PIC32MK1024MCF064 | **STM32G474CET3** (170MHz, CORDIC/FMAC, ADC×5, COMP, HRTIM) | ✔ 모터제어 전용 기능 풍부 |
| 게이트드라이버 | DRV8350RH (HW 설정) | **DRV8353SRTA (SPI 설정, CSA×3 내장)** | ✔ VDS OCP·IDRIVE·데드타임을 FW에서 설정/검증 |
| 전력 FET | BSC040N10NS5 ×6 (4mΩ) | **IAUMN10S5N016G ×12** (100V 1.6mΩ, 스위치당 2병렬) | ✔ 전류 여유 크게 증가, 개별 게이트저항 |
| 전류센싱 | INA240 in-line ×3, 2mΩ | **Low-side 1mΩ ×3 + DRV8353 CSA** | △ 샘플 타이밍 제약 (FW 처리) |
| **Vbus 측정** | ❌ 없음 | ✔ **V_BAT 분압 (470k/17.4k → 22.6mV/LSB, FS 92V)** | ✔ N25 핵심 이슈 해결 |
| **nFAULT** | RA7 (FW 미사용, PWM Fault 불가) | ✔ **PC13 = TIM1_BKIN 가능 핀** [확인] | ✔ HW 즉시 차단 가능 |
| 모터 NTC | 회로 있음, FW 미사용 | ✔ TEMP_MOTOR 커넥터 + ADC | △ pull-up 5V 문제 (H1) |
| 버스 TVS | 없음 | SMCJ70A, SMBJ58A | △ 클램프 전압 (M1) |
| 버스 캡 | 전해 + 세라믹 | **세라믹 10µF×44 (전해 없음)** | △ 핫플러그 링잉 (M1) |
| 엔코더 | iC-MU150 ×2 | iC-MU150 ×2 (SPI 공유 + ABZ) | △ 5V 로직 레벨 (H2) |
| CAN | TCAN3414 | TCAN1462 (CAN-FD) + CM choke + split term + ESD | ✔ |
| 전원 | LM5008 → 5V → 3.3V | 48V→12V(LM5169)→5V(TPSM82902)→3.3V(NCV8165), PG 시퀀스 | ✔ DRV VM = 12V |

---

## 2. 리뷰 결과 — 우선순위별

### 🔴 High (보드 손상/오동작 가능 → bring-up 전에 확인)

**H1. NTC pull-up이 +5V → ADC 입력이 VDDA(3.3V)를 넘는다**
- 회로: `TEMP_MOTOR = 5V × R_ntc / (3.3k + R_ntc)`, `TEMP_BOARD`도 R56 3.3k가 +5V에 연결됨 (넷리스트 확인)
- 회로도 주석에도 "25°C ≈ 3.8V"로 적혀 있다 → **3.3V 초과**
- 영향:
  - 모터 NTC(10k): 약 **37°C 이하에서 ADC 포화** → 저온 구간을 측정할 수 없음
  - NTC 커넥터가 빠지면 핀에 **5V 직접 인가**. 아날로그 모드 핀은 5V 내성이 보장되지 않으므로 MCU가 손상될 수 있다
  - 보드 NTC: 약 0°C 이하에서 포화
- 조치: **pull-up을 +3.3V(VDDA)로 변경** (값은 3.3k~10k 재선정). FW는 `NTC_PULLUP_MV`만 3300으로 바꾸면 된다
- 추가: 회로도 주석은 보드 NTC를 "R0 = 10kΩ, B3435"로 적었지만 부품 RT1(B57421V2222J062)은 **2.2kΩ** NTC다 → 주석/부품 중 하나 정정 필요

**H2. iC-MU150 전원 5V ↔ STM32 3.3V 로직 레벨** [확인]
- MU150 VPA/VPD = +5V → ABZ 출력, SPI MISO가 **5V 스윙**
- MCU 입력 핀: PA0, PA1, PA6, PA7 (ABZ), PA15 (Z), PB4 (MISO)
  - STM32G474에서 ADC 겸용 핀(PA0~PA7 등)은 **TT_a(3.6V 내성)**일 가능성이 높다 → 5V 입력은 절대최대정격 초과 [확인: 데이터시트 "Pin definitions" I/O structure 열]
  - PB4, PA15는 FT(5V 내성)일 가능성이 높다
- 반대 방향: MCU 3.3V 출력(SCLK/MOSI/CS)이 5V 전원 MU150의 VIH(보통 0.7×VDD = 3.5V)에 못 미칠 수 있다 [확인: iC-MU150 데이터시트 입력 레벨]
- N25(PIC32)는 해당 핀이 5V 내성이었을 수 있어서 같은 구조가 문제없이 동작했을 수 있다. **STM32에서 그대로 쓰면 안 될 수 있다**
- 조치: (a) 레벨 시프터(74LVC1T45 계열, 방향 고정형) 또는 (b) 직렬저항 + 3.3V 클램프, (c) MU150이 3.3V 동작을 지원하는지 확인

**H3. PB8-BOOT0 = EEPROM /W(WP) + 4.7k pull-up + 10k pull-down**
- 넷리스트: `PB8 – R18(10k, GND) – R65(4.7k, +3.3V) – U4.W`
- 리셋 시 BOOT0 ≈ 3.3 × 10/14.7 = **2.24V** → 논리 레벨이 애매 → **시스템 부트로더로 부팅할 수 있음** (필드에서 간헐적인 "부팅 안 됨")
- 조치(FW, 즉시): Option Byte `nSWBOOT0=0, nBOOT0=1` → BOOT0 핀 무시 ([CubeMX_Setup §12](../N25_STM_FW/docs/CubeMX_Setup.md))
- 조치(HW, 다음 리비전): WP를 다른 GPIO로 분리하거나 BOOT0 풀다운만 남기기

**H4. nFAULT pull-up = DVDD(5V 추정) → PC13 입력** [확인]
- R44 10k가 DRV8353 DVDD 핀에 연결됨. DVDD는 DRV 내부 LDO 출력이다 (5V 계열 [확인])
- PC13이 FT(5V 내성) 핀인지 확인 필요. 아니면 pull-up을 +3.3V로 변경
- PC13이 **TIM1_BKIN AF를 지원하는지** CubeMX에서 확인 (지원하면 nFAULT → HW PWM 차단 경로 완성)

### 🟠 Medium

**M1. 회생/서지 대책 — TVS 클램프 전압과 전부 세라믹인 버스**
- D5 **SMCJ70A**: 항복 77.8V, 클램프 ≈ **113V @Ipp** → FET 100V 정격을 넘으므로 FET 보호가 안 됨
- D33 **SMBJ58A**: 항복 64.4V, 클램프 93.6V (600W급 소형) → 짧은 서지만 흡수, 회생 에너지 흡수는 불가
- 벌크 캡: **10µF 1210 X7R ×44**, 전해 없음 → 48V DC bias에서 실효 용량 약 30~40% → **150µF 내외**
  - 세라믹만 있으면 Q가 높다 → 배터리 케이블 L과 공진하여 **핫플러그 시 입력전압이 최대 2배 가까이 링잉**할 수 있다 (48V → 90V대)
- 조치:
  - 핫플러그 시 V_BAT 피크 실측 (차동 프로브). 필요하면 전해 캡(ESR 댐핑) 또는 RC 스너버 추가
  - FW: Vbus 과전압 시 회생 토크 제한 (이제 측정 가능 ✔), 다음 단계에서 구현
  - 큰 회생이 예상되면 브레이크 초퍼 검토

**M2. Low-side shunt 전류 측정 제약**
- low-side FET가 켜져 있을 때만 전류가 흐른다 → **카운터 peak(low-side ON 중앙)에서 샘플** (FW 반영)
- 듀티가 높으면(변조율 ↑) low-side ON 시간이 짧아 측정할 수 없음 → 다음 단계 FOC에서 **듀티가 가장 낮은 2상만 쓰고 3번째는 계산**하는 2상 재구성 필요
- 3상 합 진단은 듀티 ≤ 85%일 때만 유효 (FW 반영)

**M3. CAN Split termination 상시 장착 (R71/R72 60Ω)**
- 버스 중간 노드까지 종단하면 종단 저항이 병렬로 늘어나 신호 진폭이 감소한다
- AMR에서 드라이버 2개 + 상위가 한 버스면 **양 끝만 종단**해야 함 → 점퍼/DNP 옵션 검토

**M4. SPI1 버스에 4개 디바이스 + 디버그 커넥터 공유**
- DRV8353(Mode 1), EEPROM(Mode 0), MU150 ×2(Mode 0), J1(외부 MU 설정 툴)
- 외부 툴과 MCU가 동시에 구동하면 버스 충돌 → FW에 `BSP_SPI_ReleaseBus()` 구현 (CMD 0x43)
- 이름 혼동: `SPI2_CS_MU`는 SPI2가 아니라 SPI1의 두 번째 CS다 → 다음 리비전에 `SPI1_CS_MU2`로 이름 정리 권장

**M5. 중복 부품 번호 (BOM 오류 위험)**
- **R53, R55**가 두 번씩 쓰임 (470kΩ 3216 / 0Ω 1005, 17.4kΩ 3216 / 0Ω 1005) → V_BAT 분압 옵션
- **D35**가 두 번씩 쓰임 (PESD2CANFD24V CAN TVS / LED RED)
- C13 값 미기재

**M6. 전류 센싱 이득 선택**
| CSA Gain | mA/LSB | 측정 범위 | 비고 |
|----------|--------|-----------|------|
| 10 | 80.6 | ±165A | 해상도 부족 |
| **20 (FW 기본)** | **40.3** | **±82A** | 12FET 피크 여유 |
| 40 | 20.1 | ±41A | N25와 동일 해상도, 피크 부족 가능 |
→ 모터 피크 전류 요구치가 정해지면 확정 (`board_config.h`의 `CUR_CSA_GAIN`, `dev_drv8353` 설정)

### 🟢 Low / 참고

- **L1** PC14(LED_B)는 백업 도메인 핀이라 싱크 전류가 3mA로 제한된다 → 1kΩ LED(약 1.3mA)는 OK. LSE는 사용하지 말 것
- **L2** VREF+ = VDDA(LDO NCV8165) → 전류/Vbus 스케일 정확도는 LDO 정확도(±1~2%)를 따른다 → VREFINT로 보정 가능
- **L3** SWO 핀(PB3)을 SPI1_SCK로 사용, UART 없음 → printf 디버그 불가 → **CAN 텔레메트리** 또는 SEGGER RTT 사용
- **L4** PA15(ENC_I)는 리셋 시 JTDI(pull-up) → SWD만 쓰면 문제 없음
- **L5** ENC2(U10)는 Index 미연결 → 보조 엔코더는 SPI 절대값으로 원점

### ✔ 잘 된 점
- ENABLE 10k pull-down (리셋 중 게이트 OFF 보장), FAULT/ENABLE LED
- FET 개별 게이트 저항 + 턴오프 다이오드 (병렬 FET 전류 분배)
- 전원 PG 시퀀싱 (12V → 5V → 3.3V)
- CAN: CM choke + split termination + ESD, 디버그/엔코더 라인 ESD
- Vbus 측정, 모터 온도 입력, DRV8353 SPI → **N25 고장 원인으로 지목된 "보호 공백"을 HW 측면에서 대부분 해소**

---

## 3. 핀 / 주변장치 할당표 (펌웨어 기준)

| 기능 | 핀 | 주변장치 | 비고 |
|------|----|----------|------|
| PWM U/V/W 상단 | PA8 / PA9 / PA10 | TIM1_CH1/2/3 | |
| PWM U/V/W 하단 | PB13 / PB14 / PB15 | TIM1_CH1N/2N/3N | |
| ADC 트리거 | — | TIM1_CH4 → TRGO2 | 출력 없음 |
| nFAULT | PC13 | TIM1_BKIN | [확인] |
| DRV ENABLE | PA3 | GPIO | |
| 전류 U / V / W | PB1 / PB12 / PA2 | ADC3_IN1 / ADC4_IN3 / ADC1_IN3 (injected) | 동시 샘플 |
| Vbus / T_board / T_motor | PA4 / PB2 / PB11 | ADC2_IN17 / IN12 / IN14 (DMA) | |
| ENC1 A/B/Z | PA0 / PA1 / PA15 | TIM2 (32bit) / TIM2_ETR | 레벨 확인(H2) |
| ENC2 A/B | PA6 / PA7 | TIM3 | 레벨 확인(H2) |
| SPI1 SCK/MISO/MOSI | PB3 / PB4 / PB5 | SPI1 | 4 디바이스 공유 |
| CS: DRV / ROM / MU1 / MU2 | PB0 / PB6 / PA5 / PB10 | GPIO | |
| EEPROM WP | PB8 | GPIO | BOOT0 겸용(H3) |
| CAN RX/TX | PA11 / PA12 | FDCAN1 | |
| 브레이크 | PB9 | TIM17_CH1 | [확인] |
| LED A / B | PB7 / PC14 | GPIO | |
| SWD | PA13 / PA14 | | |
| HSE | PF0 | 24MHz 발진기 (bypass) | |
| 1kHz 스케줄러 | — | TIM6 | |
| Watchdog | — | IWDG (~250ms) | |

**남는 주요 자원** (다음 단계에서 활용): CORDIC(sin/cos·atan2 가속), FMAC(필터), COMP×7 + DAC(HW 과전류 비교기 → TIM1_BKIN2), ADC5, HRTIM, OPAMP.
