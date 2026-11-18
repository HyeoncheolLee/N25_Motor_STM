# N25 Motor Controller — STM32 전환 및 모터 제어 스터디

AMR 휠 구동용 3상 BLDC(PMSM) 모터 드라이버 **N25**를 연구·재설계하는 저장소다.
- 기존 **PIC32MK 펌웨어**를 분석해 필드 고장(BIG/CUR/JAM 에러, 게이트드라이버·FET 소손, 코일 손상, 링자석 탈락)의 원인을 정리했다.
- 신규 **STM32G474 보드**용 펌웨어를 처음부터 작성한다 (현재 Bring-up 단계).

| 항목 | 내용 |
|------|------|
| 신규 MCU | STM32G474CET3 (170MHz, Cortex-M4F) |
| 게이트드라이버 | TI DRV8353S (SPI, CSA ×3) |
| 전력단 | IAUMN10S5N016G ×12 (100V, 스위치당 2병렬), low-side shunt 1mΩ ×3 |
| 엔코더 | iC-MU150 ×2 (SPI 절대값 + ABZ) |
| 통신 | CAN (FDCAN1, 1Mbps classic) |
| 개발환경 | STM32CubeIDE + HAL |

---

## 현재 상태 (2026-11-19)

| 단계 | 상태 |
|------|------|
| 기존 N25(PIC32) FW 분석 / 고장 원인 검토 | ✅ 완료 → `Study/03`, `Study/04` |
| 신규 STM 회로도 리뷰 | ✅ 완료 → `Study/06` (HW 확인 필요 항목 있음) |
| Bring-up용 Basic Software 작성 | ✅ 작성 완료, **⚠ 미빌드 / 미검증** |
| CubeIDE 프로젝트(.ioc) 생성 | ⬜ 해야 함 → `N25_STM_FW/docs/CubeMX_Setup.md` |
| Bring-up B0 ~ B10 | ⬜ |
| 모터 제어 (FOC, 속도/위치, 보호, DOB) | ⬜ |

👉 **남은 작업 전체 목록: [TODO.md](TODO.md)**

---

## 저장소 구조

```
.
├── README.md                ← 이 문서
├── TODO.md                  ← 남은 작업 체크리스트 (여기서부터 이어서 진행)
├── CLAUDE.md                ← AI 어시스턴트(Claude Code)용 프로젝트 컨텍스트
│
├── N25_STM_FW/              ← ★ 신규 STM32 펌웨어
│   ├── README.md
│   ├── docs/CubeMX_Setup.md    CubeMX 설정표 (클록/핀/주변장치/NVIC/Option Byte)
│   └── User/                   직접 작성한 코드 (CubeMX 재생성과 무관)
│       ├── Config/   board_config.h (HW 상수/스케일), fw_version.h
│       ├── Common/   util_maf (bit-shift 이동평균), util_ringbuf, util_crc
│       ├── Bsp/      bsp_pwm/adc/spi/can/encoder/gpio/brake  (HAL 래퍼)
│       ├── Device/   dev_drv8353, dev_mu150, dev_m95128      (외부 IC)
│       ├── Service/  svc_fault/param/monitor/comm/sched      (공통 서비스)
│       └── App/      app_main, app_bringup, app_motor.h(설계 초안)
│
├── Study/                   ← 스터디 노트 (한글)
│   ├── 00_README.md            목차
│   ├── 01_BLDC_개발_프로세스.md
│   ├── 02_N25_HW_분석.md        (기존 PIC32 보드)
│   ├── 03_N25_FW_구조_분석.md   (기존 FW)
│   ├── 04_문제원인_분석_및_개선방안.md
│   ├── 05_모터제어_이론_노트.md
│   ├── 06_N25_STM_HW_리뷰.md    (신규 보드)
│   └── 07_N25_STM_FW_구조_및_Bringup.md
│
└── N25/                     ← 원본 자료 (참고용, 수정하지 않음)
    ├── N25_Schematic PDF.pdf        기존 PIC32 보드 회로도
    ├── N25_STM_Schematic PDF.pdf    신규 STM32 보드 회로도
    └── AMR_MC_N25-PositionCtrl/     기존 PIC32 펌웨어 (PositionCtrl 브랜치 스냅샷)
```

---

## 다른 PC에서 이어서 작업하기

```bash
git clone https://github.com/HyeoncheolLee/N25_Motor_STM.git
cd N25_Motor_STM
```

1. **STM32CubeIDE** 설치 (1.15 이상 권장, STM32G4 패키지 포함)
2. [N25_STM_FW/docs/CubeMX_Setup.md](N25_STM_FW/docs/CubeMX_Setup.md) 대로 `N25_STM_FW/` 위치에 새 프로젝트를 만든다
   - 프로젝트 위치를 저장소 안 `N25_STM_FW/`로 지정하면 `User/` 폴더와 같은 곳에 `Core/`, `Drivers/`, `.ioc`가 생긴다
   - 생성된 `.ioc`, `.project`, `.cproject`, `Core/`, `Drivers/`도 **커밋**한다 (다른 PC에서 바로 빌드하기 위해)
3. Include path에 `User/*` 추가 → `main.c`에 `App_Init()` / `App_Loop()` → 빌드
4. [TODO.md](TODO.md)의 다음 항목부터 진행

Claude Code로 이어서 작업한다면 저장소 루트에서 실행하자. [CLAUDE.md](CLAUDE.md)가 자동으로 읽혀 프로젝트 맥락(규칙, 진행 상황, 주의사항)이 전달된다.

---

## 핵심 설계 원칙 (요약)

- **계층 구조**: App → Service → Device → BSP → HAL. include는 아래 방향만 허용, 위로는 콜백/weak hook
- **이름 규칙**: `bsp_pwm.c` / `BSP_PWM_OutputEnable()`, `svc_fault.c` / `SVC_Fault_Set()`, `dev_mu150.c` / `MU150_ReadPosition()`, 변수에 단위 접미어(`_mA`, `_mV`, `_x10`, `_permil`, `_Q16`)
- **센싱**: bit-shift MAF (`sum >> shift`), 제어 피드백에는 미사용
- **보호 우선**: nFAULT→TIM1 Break(HW 차단), 20kHz 순시 과전류, 3상 합 진단, Vbus OV/UV, 온도, 클리어 반복 제한, 최초 Fault 기록
- **실행 규칙**: 20kHz ISR에서는 SPI/EEPROM/CAN 송신 금지. SPI 절대값은 main loop에서 정합성 감시용으로만

자세한 내용은 [Study/07](Study/07_N25_STM_FW_구조_및_Bringup.md) 참고.
