# N25_STM_FW — STM32G474 모터 드라이버 펌웨어 (Bring-up 단계)

> 버전 0.1.0 (Bring-up) · MCU STM32G474CETx · 회로 `N25/N25_STM_Schematic PDF.pdf`
> 상태: **코드 작성 완료, 미빌드/미검증** (작성 환경에 ARM 툴체인 없음 → CubeIDE에서 첫 빌드 필요)

## 1. 폴더 구조

```
N25_STM_FW/
├── docs/
│   └── CubeMX_Setup.md        ← .ioc 설정 표 (먼저 읽기)
├── (CubeIDE 생성) Core/ Drivers/ N25_STM_FW.ioc
└── User/                      ← 직접 작성하는 코드 (CubeMX 재생성에 영향 없음)
    ├── Config/   board_config.h  fw_version.h
    ├── Common/   util_maf  util_ringbuf  util_crc
    ├── Bsp/      bsp  bsp_gpio  bsp_spi  bsp_pwm  bsp_adc  bsp_encoder  bsp_can  bsp_brake
    ├── Device/   dev_drv8353  dev_mu150  dev_m95128
    ├── Service/  svc_sched  svc_fault  svc_param  svc_monitor  svc_comm
    └── App/      app_main  app_bringup  (app_motor.h: 다음 단계 설계 초안)
```

## 2. 계층 구조

```
 ┌──────────────────────────────────────────────────────────────┐
 │ App      app_main (상태머신/부팅/태스크), app_bringup (시험모드) │
 ├──────────────────────────────────────────────────────────────┤
 │ Service  svc_fault  svc_param  svc_monitor  svc_comm  svc_sched│
 ├──────────────────────────────────────────────────────────────┤
 │ Device   dev_drv8353  dev_mu150  dev_m95128   (외부 IC 드라이버) │
 ├──────────────────────────────────────────────────────────────┤
 │ BSP      bsp_pwm  bsp_adc  bsp_spi  bsp_can  bsp_encoder ...    │
 ├──────────────────────────────────────────────────────────────┤
 │ STM32 HAL (CubeMX 생성)                                         │
 └──────────────────────────────────────────────────────────────┘
   Common(util_*), Config 는 모든 계층에서 사용 가능
```
- 위 계층만 아래 계층을 호출한다 (`#include` 방향도 동일).
- 아래 계층이 위로 알려야 할 때는 **콜백 등록** 또는 **weak hook**을 쓴다.
  - `BSP_ADC_RegisterFastCallback()`: 20kHz Fast Loop → App
  - `BSP_PWM_OnBreak()` (weak): nFAULT Break → svc_fault
  - `BSP_OnTick1ms()` (weak): TIM6 → svc_sched
  - `App_OnCommand()` (weak): CAN 명령 → App

## 3. 실행 구조

| 컨텍스트 | 주기 | 내용 |
|----------|------|------|
| TIM1 BRK ISR | 즉시 | nFAULT → HW가 PWM 차단 → Fault 기록 |
| ADC1 JEOS ISR | 20kHz | 3상 전류 → 엔코더 갱신 → **보호(순시 과전류, 3상 합)** → 제어 |
| main: 1ms 태스크 | 1kHz | Fault 관리, Vbus/온도(MAF), CAN 명령, 브레이크, 상태머신, WDG |
| main: 10ms | 100Hz | DRV8353 Fault 레지스터 읽기 |
| main: 100ms | 10Hz | LED, 엔코더 정합성(ABZ vs SPI) |
| main: 1000ms | 1Hz | Heartbeat |

## 4. 시작하기

1. [docs/CubeMX_Setup.md](docs/CubeMX_Setup.md) 대로 CubeIDE 프로젝트 생성 → `User/` 추가 → `main.c`에 `App_Init()` / `App_Loop()` 두 줄
2. 빌드 → 컴파일 에러 수정 (HAL 버전 차이로 매크로명 등이 다를 수 있음)
3. **Option Bytes 설정 (nSWBOOT0=0, nBOOT0=1)**
4. [Bring-up 절차](../Study/07_N25_STM_FW_구조_및_Bringup.md#5-bring-up-절차)를 순서대로 진행

## 5. 코딩 규칙 (요약)

| 대상 | 규칙 | 예 |
|------|------|-----|
| 파일 | `<계층접두어>_<모듈>.c/h` 소문자 | `bsp_pwm.c`, `svc_fault.h`, `dev_mu150.c` |
| 함수 | `<계층>_<모듈>_<동작>()` | `BSP_PWM_OutputEnable()`, `SVC_Fault_Set()` |
| 디바이스 함수 | `<칩이름>_<동작>()` | `DRV8353_ReadReg()`, `MU150_ReadPosition()` |
| 구조체 / enum | `_t` / `_e` 접미어 | `MAF_Handle_t`, `Fault_Id_e` |
| 파일 static 변수 | `s_` 접두어 | `s_fault_cnt` |
| 단위 | 이름 끝에 단위 | `vbus_mV`, `iu_mA`, `temp_x10`, `duty_permil`, `_Q16` |
| 매크로 상수 | 대문자 + 단위 | `PWM_FREQ_HZ`, `PROT_VBUS_OV_TRIP_MV` |
| 하드웨어 값 | `board_config.h` 한 곳에만 | |

자세한 내용은 [Study/07](../Study/07_N25_STM_FW_구조_및_Bringup.md)에 있다.
