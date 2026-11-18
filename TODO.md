# TODO — 남은 작업 목록

> 마지막 갱신: 2026-11-19
> 완료하면 `[x]`로 바꾸고, 측정값이나 결정사항은 해당 항목 아래에 적는다.
> 우선순위: 🔴 다음 단계 진행을 막음 · 🟠 이번 단계 안에 · 🟢 나중에

---

## 0. 지금 바로 할 일 (Next Action)

1. 🔴 [1.1](#11-cubeide-프로젝트-생성) CubeIDE 프로젝트 생성 → 첫 빌드
2. 🔴 [2.1](#21-hw-확인-bring-up-전) HW 확인 항목 H2(엔코더 로직 레벨), H3(BOOT0), H4(nFAULT 풀업)
3. 🔴 [3](#3-bring-up-b0--b10) Bring-up B0부터 진행

---

## 1. 프로젝트 환경

### 1.1 CubeIDE 프로젝트 생성
- [ ] 🔴 `N25_STM_FW/`에 STM32G474CETx 프로젝트 생성 ([CubeMX_Setup.md](N25_STM_FW/docs/CubeMX_Setup.md) §0~§10)
- [ ] 🔴 CubeMX가 다음 AF/채널을 실제로 제안하는지 확인 (문서의 [확인] 표시 항목)
  - [ ] PC13 → TIM1_BKIN
  - [ ] PB1 → ADC3_IN1, PB12 → ADC4_IN3, PA4 → ADC2_IN17
  - [ ] PB9 → TIM17_CH1
  - [ ] PA15 → TIM2_ETR (엔코더 index, 나중에)
  - 다르면 `bsp_adc.c` / `bsp_pwm.c` / `bsp_brake.c`의 핸들·채널 수정 + CubeMX_Setup.md 갱신
- [ ] 🔴 `User/` source location + include path 등록, `main.c`에 `App_Init()` / `App_Loop()` 추가 (§11)
- [ ] 🔴 **첫 빌드 → 컴파일 에러/경고 0** (작성 환경에 툴체인이 없어 한 번도 빌드되지 않은 코드)
  - 예상 수정 포인트: HAL 버전별 매크로명(`FDCAN_DLC_BYTES_x`, ADC 트리거 이름), `volatile` 캐스트 경고, CubeMX 생성 함수명
- [ ] 🟠 `.ioc`, `.project`, `.cproject`, `Core/`, `Drivers/` 커밋
- [ ] 🟢 빌드 설정: `-Wall -Wextra`, Release 최적화 `-O2`, FPU hard ABI 확인
- [ ] 🟢 CAN 시험용 PC 툴 결정 (PCAN-View / python-can 스크립트) → `tools/`에 명령 스크립트 저장

### 1.2 저장소 운영
- [ ] 🟢 브랜치 규칙: `main`(검증된 것) / `dev` / `feature/<기능>`
- [ ] 🟢 버전 태그: Bring-up 완료 시 `v0.1.0`, FOC 완료 시 `v0.2.0` (`fw_version.h`와 맞추기)

---

## 2. 하드웨어

### 2.1 HW 확인 (bring-up 전)
상세: [Study/06](Study/06_N25_STM_HW_리뷰.md)
- [ ] 🔴 **H2** iC-MU150(5V 전원) 출력 → STM32 입력 핀(PA0/PA1/PA6/PA7/PA15/PB4) 허용 전압 확인 (데이터시트 I/O structure: FT vs TT_a)
  - [ ] MCU 3.3V 출력(SCLK/MOSI/CS)이 MU150 VIH를 만족하는지 확인
  - 결과: ____
- [ ] 🔴 **H3** Option Byte `nSWBOOT0=0`, `nBOOT0=1` 설정 (BOOT0 핀 ≈2.2V 문제 회피)
- [ ] 🔴 **H4** DRV8353 DVDD 전압 측정 → PC13 5V 허용 여부 확인
- [ ] 🟠 **H1** NTC 풀업 +5V → ADC 3.3V 초과 (모터 NTC 37°C 이하 포화). HW 수정 전까지 FW는 포화를 "저온"으로 처리 중
- [ ] 🟠 **M1** 48V 핫플러그 시 V_BAT 피크 측정 (차동 프로브) → 링잉 크기 기록: ____ V
- [ ] 🟠 보드 NTC 부품(2.2kΩ, B57421V2222) B값 확인 → `board_config.h` `NTC_BOARD_BETA`
- [ ] 🟢 CAN 종단 위치 결정 (드라이버 2개 + 상위 → 양 끝만)

### 2.2 다음 HW 리비전 반영 목록
- [ ] NTC 풀업 → 3.3V (VDDA)
- [ ] MU150 ↔ MCU 레벨 시프터 (H2 결과에 따라)
- [ ] EEPROM WP와 BOOT0 분리
- [ ] nFAULT 풀업 → 3.3V (H4 결과에 따라)
- [ ] 버스 TVS 재선정 (클램프 < 100V), 전해 캡 또는 스너버 추가 검토, 브레이크 초퍼 검토
- [ ] CAN 종단 선택 가능하게 (점퍼/DNP)
- [ ] 중복 부품번호 정리: R53, R55, D35 / C13 값 기재 / `SPI2_CS_MU` → `SPI1_CS_MU2` 이름 정리
- [ ] (선택) 디버그 UART 또는 SWO 핀 확보

---

## 3. Bring-up (B0 ~ B10)

절차와 통과 기준: [Study/07 §5](Study/07_N25_STM_FW_구조_및_Bringup.md#5-bring-up-절차)

| # | 항목 | 상태 | 결과 / 메모 |
|---|------|------|-------------|
| B0 | 전원 레일, SWD, Option Byte, 엔코더 신호 레벨 | ⬜ | |
| B1 | 클록 170MHz, LED 1Hz, Heartbeat 0x701 | ⬜ | |
| B2 | CAN PING / FAULT_GET | ⬜ | |
| B3 | EEPROM 파라미터 저장/로드/뱅크 복구 | ⬜ | |
| B4 | Vbus(±2%), 보드/모터 온도 | ⬜ | |
| B5 | DRV8353 레지스터 read-back, nFAULT High | ⬜ | |
| B6 | 엔코더: 16384 cnt/rev, 방향, ABZ↔SPI 정합성 | ⬜ | |
| B7 | 전류 오프셋 2048±150, 노이즈 < 0.2A | ⬜ | |
| B8 | PWM 20kHz, 데드타임, 링잉, 자동 OFF | ⬜ | |
| B9 | Fault 주입 (nFAULT, UV/OV, 클리어 제한, WDG) | ⬜ | |
| B10 | 모터: 전압벡터 정렬, 전류 부호, 극쌍수, 전기각 오프셋, 오픈루프 회전 | ⬜ | |

Bring-up 중 확정해서 코드/파라미터에 반영할 값:
- [ ] 전류 부호 (`bsp_adc.c` `ADC_RawToMilliAmp`)
- [ ] 엔코더 방향 (TIM2/TIM3 IC polarity) 및 `PARAM_MOTOR_DIR`
- [ ] MU150 SPI 프레임 형식: 위치 비트수(`MU150_POS_BITS`), nERR/nWARN 위치 (`dev_mu150.c` TODO(HW))
- [ ] 전기각 오프셋 → `PARAM_ENC_ELEC_OFFSET_CNT`
- [ ] 엔코더 정합성 임계 → `PARAM_ENC_MISMATCH_MAX_CNT` (기본 0=OFF → 40 권장)
- [ ] DRV8353 IDRIVE / VDS_LVL / DEAD_TIME (게이트 파형 보고 결정)
- [ ] CSA 이득 (20 vs 40) — 모터 피크 전류 요구치에 따라
- [ ] SPI 클록 상향 (1.33MHz → 5.3MHz) 후 통신 안정성
- [ ] Fast Loop ISR 실행시간 (DIAG) : ____ cycles / 8500

---

## 4. 모터 제어 (Phase M)

설계 방향: [Study/07 §6](Study/07_N25_STM_FW_구조_및_Bringup.md#6-다음-단계-phase-m-설계-방향), [app_motor.h](N25_STM_FW/User/App/app_motor.h)

- [ ] 🔴 **M0 요구사항 정의**: 정격/피크 토크·전류, 최고속도, 로봇 최대 적재 질량, 바닥 마찰계수 → 슬립 한계 전류 `I_slip`, 코일 연속 정격, 자석 허용 온도 ([Study/01 §0](Study/01_BLDC_개발_프로세스.md))
- [ ] **M1 모터 상수 측정 기능**: R(DC 전압 스텝), L(전류 상승 기울기), Ke(오픈루프 회전 BEMF), J(가감속), 열 시정수
- [ ] **M2 FOC 전류 루프** `User/Ctrl/ctrl_foc`, `ctrl_pi`
  - [ ] Clarke/Park (CORDIC 가속 검토)
  - [ ] Low-side 2상 재구성 (듀티가 가장 높은 상 제외)
  - [ ] PI: `Kp = L·ωc`, `Ki = R·ωc`, **올바른 스케일의 anti-windup**
  - [ ] Vbus 실측 보상 SVPWM, 전압 원형 제한
  - [ ] Iq step 응답, 각도별 동일성 시험
- [ ] **M3 속도 루프** `ctrl_speed`: PLL/관측기 속도 추정, 속도 PI + 포화 anti-windup
- [ ] **M4 위치/AMR 지령** `ctrl_pos`: 위치 P, **추종오차 클램프 + 포화 시 기준 적분 정지** (N25 windup 대응), CAN 타임아웃 시 감속 정지
- [ ] **M5 보호 강화**: I²t 열 모델, 온도 디레이팅, **Vbus 상승 시 회생 토크 제한**, 스톨 검출(수백 ms)
- [ ] **M6 DOB**: 부하토크 추정 → 슬립/스톨 감지 → 토크 제한 + CAN 상태 비트
- [ ] **M7 HW 과전류 비교기**: COMP + DAC → TIM1_BKIN2 (U상 PB1 = COMP1_INP 가능 여부 확인)
- [ ] **M8 AMR 상위 연동**: 기존 N25 CAN 메시지 호환 계층 (속도 지령 dlc=8/4, 엔코더 피드백, 상태 0x...)

---

## 5. 검증 / 원인 확정 (Phase V)

[Study/04 §5](Study/04_문제원인_분석_및_개선방안.md) 시험 계획
- [ ] T1 바퀴 락 (적재) — 오차 누적, 포화 시간, 트립 시간, 코일 온도
- [ ] T2 슬립 재현 (저마찰 판) — 그립 복귀 시 전류 스파이크
- [ ] T3 회생 (급정지, 회생 흡수 불가 전원) — Vbus 피크
- [ ] T4 주행 중 배터리 커넥터 순간 개방 — Vbus 피크
- [ ] T5 저전압 상간 단락 — DRV OCP, nFAULT, FW 반응
- [ ] T6 장시간 고부하 후 ABZ vs 절대값 차이
- [ ] T7 N25 고장품 분해 분석 (FET 손상 형태, 코일 변색 상, 자석 접착부)
- [ ] 동일 시나리오를 N25(PIC32)와 N25_STM에서 비교

---

## 6. 결정 대기 / 질문

- [ ] CAN 프로토콜: 신규(CANopen 유사) 유지 + 호환 계층? 아니면 기존 N25 형식 그대로?
- [ ] 위치 제어 vs 속도 제어: AMR 상위가 휠에 위치(적분) 지령을 계속 줄지, 속도 지령 + 오도메트리 피드백으로 바꿀지 (상위 팀과 협의)
- [ ] 브레이크 정책: Fault 시 즉시 체결? 감속 후 체결?
- [ ] 펌웨어 업데이트 방식: SWD만? CAN 부트로더 필요?
