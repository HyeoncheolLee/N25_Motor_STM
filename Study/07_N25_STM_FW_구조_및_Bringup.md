# 07. N25_STM 펌웨어 구조 및 Bring-up 절차

> 코드: [N25_STM_FW/](../N25_STM_FW/) · CubeMX 설정: [CubeMX_Setup.md](../N25_STM_FW/docs/CubeMX_Setup.md) · HW 리뷰: [06](06_N25_STM_HW_리뷰.md)
> 상태: **Bring-up용 Basic Software 작성 완료, 미빌드/미검증**

---

## 1. 개발 단계 로드맵

```
[Phase B] Bring-up (지금)                       [Phase M] 모터 제어 (다음)
 B0 Option Byte / 전원 / SWD                     M1 R, L, Ke 측정 → 전류 PI 설계
 B1 Clock, LED, Heartbeat                        M2 FOC 전류루프 (Id/Iq), 2상 재구성
 B2 CAN 통신 (PING)                              M3 속도 추정 + 속도 PI
 B3 EEPROM / 파라미터                            M4 위치 P + 추종오차 제한 (AMR 지령)
 B4 Vbus / 온도 (MAF)                            M5 I²t, 온도 디레이팅, 회생 제한
 B5 DRV8353 SPI 설정·검증                         M6 DOB → 부하토크 / 슬립 감지
 B6 엔코더 (ABZ + SPI 절대값 정합성)               M7 AMR 상위 프로토콜 호환 계층
 B7 전류 오프셋 / 노이즈                          M8 실주행 / 내구 / Fault 주입
 B8 PWM 파형 (모터 없이)
 B9 Fault 주입 (nFAULT, OC, OV, 클리어 제한)
 B10 모터 연결: 전압벡터 정렬 → 오픈루프 회전
```
문서 [01](01_BLDC_개발_프로세스.md)의 게이트 G1~G6이 Phase B에, G7~G10이 Phase M에 해당한다.

---

## 2. 소프트웨어 아키텍처

### 2.1 계층과 의존 방향

```
 App      ── 무엇을 할지 (부팅 순서, 상태 머신, 시험 모드, (다음) 모터 제어)
   ↓
 Service  ── 보드와 무관한 공통 기능 (Fault, 파라미터, 모니터링, 통신 프로토콜, 스케줄러)
   ↓
 Device   ── 외부 IC 드라이버 (DRV8353, iC-MU150, M95128) — 칩 데이터시트 지식
   ↓
 BSP      ── 이 보드의 MCU 주변장치 사용법 (핀, 타이머, ADC 채널) — HAL 을 직접 쓰는 유일한 계층
   ↓
 HAL      ── ST 제공 (CubeMX 생성)
```
- **include는 아래 방향만 허용**한다. 위로 알려야 할 때는 콜백 등록이나 weak hook을 쓴다.
- 그래서 회로가 바뀌면 BSP만, 게이트드라이버 칩이 바뀌면 Device만, 프로토콜이 바뀌면 Service만 고치면 된다.

AUTOSAR의 MCAL / ECU Abstraction / Service / Application 구분을 소형 MCU에 맞게 단순화한 형태로, 업계에서 흔히 쓰는 방식이다.

### 2.2 파일 / 이름 규칙

| 계층 | 폴더 | 파일 | 함수 | 예 |
|------|------|------|------|-----|
| Config | `User/Config` | `board_config.h`, `fw_version.h` | (매크로) | `PWM_FREQ_HZ`, `CUR_MA_PER_LSB_Q16` |
| Common | `User/Common` | `util_<기능>.c/h` | `<모듈>_<동작>` | `MAF_Update()`, `RingBuf_Push()`, `CRC16_Calc()` |
| BSP | `User/Bsp` | `bsp_<주변장치>.c/h` | `BSP_<모듈>_<동작>` | `BSP_PWM_OutputDisable()`, `BSP_ADC_GetSlowRaw()` |
| Device | `User/Device` | `dev_<칩>.c/h` | `<칩>_<동작>` | `DRV8353_WriteReg()`, `MU150_ReadPosition()`, `EEPROM_Write()` |
| Service | `User/Service` | `svc_<기능>.c/h` | `SVC_<모듈>_<동작>` | `SVC_Fault_Set()`, `SVC_Param_Get()` |
| App | `User/App` | `app_<기능>.c/h` | `App_<모듈>_<동작>` | `App_Init()`, `App_BU_SetMode()` |
| (다음) Ctrl | `User/Ctrl` | `ctrl_<제어기>.c/h` | `Ctrl_<모듈>_<동작>` | `Ctrl_PI_Update()`, `Ctrl_FOC_Run()` |

| 대상 | 규칙 | 예 |
|------|------|-----|
| 구조체 / enum 타입 | `<모듈>_<이름>_t` / `_e` | `MAF_Handle_t`, `BSP_Led_e`, `Fault_Id_e` |
| 상태를 가진 객체 | HAL처럼 `Handle_t` + 첫 인자 포인터 | `MAF_Update(&s_vbus_maf, x)` |
| 파일 내부 변수 | `static` + `s_` 접두어 | `s_break_cnt` |
| 전역 변수 | 사용 금지 (getter 함수 제공) | `SVC_Mon_GetVbus_mV()` |
| 단위 | 변수/매크로 이름 끝에 표기 | `_mA`, `_mV`, `_ms`, `_cnt`, `_x10`(0.1단위), `_permil`, `_Q16`(고정소수점) |
| 매크로 | 대문자 + 모듈 접두어 | `PROT_VBUS_OV_TRIP_MV`, `MU150_OP_SDAD` |
| 확인 필요 | 주석 `TODO(HW)` | |

### 2.3 실행 컨텍스트와 규칙

| 컨텍스트 | 주기 | 허용 | 금지 |
|----------|------|------|------|
| TIM1 BRK ISR | 이벤트 | Fault 기록 | — |
| ADC ISR (Fast Loop) | 20kHz (50µs) | 레지스터 접근, 계산, `SVC_Fault_Set` | **SPI, EEPROM, CAN 송신, HAL_Delay, 블로킹 대기** |
| CAN RX ISR | 이벤트 | 링버퍼 push | 명령 처리 |
| TIM6 ISR | 1kHz | 태스크 깃발 set | 태스크 실행 |
| main loop 태스크 | 1/10/100/1000ms | SPI, EEPROM, CAN, 블로킹(짧게) | 50ms 이상 블로킹 (WDG 250ms) |

- **SPI는 main loop에서만** 쓴다. SPI1 버스에 디바이스 4개가 모드를 바꿔 가며 공유하므로 ISR에서 끼어들면 프레임이 깨진다. `BSP_SPI_Select()`가 재진입을 감지해 `HAL_BUSY`를 반환한다.
- 20kHz 제어에는 ABZ(타이머 카운터)만 쓰고, SPI 절대값은 10Hz 정합성 감시에 쓴다.
- ISR 실행시간은 DWT 사이클 카운터로 측정해 `TELEM_DIAG`로 보낸다 (170MHz에서 8500 사이클 = 50µs가 한계, 목표 < 50%).

---

## 3. 모듈별 설명

### 3.1 Bit-shift MAF (`util_maf`)
```
N = 2^shift,   sum += x[n] − x[n−N],   y = sum >> shift
```
- 나눗셈 없이 매 샘플 덧셈 1, 뺄셈 1, 시프트 1 → 20kHz ISR에서도 부담이 없다
- `MAF_Reset(h, 첫샘플)`로 버퍼를 채워 시작 램프를 없앤다
- 군지연 = (N−1)/2 샘플 → **제어 피드백에는 쓰지 않는다** (모니터링, 보호, 표시용)
- 음수 입력의 `>>`는 −∞ 방향 내림 → 표시용은 `MAF_GetRounded()`

| 적용 위치 | 샘플링 | shift (N) | 지연 | 목적 |
|-----------|--------|-----------|------|------|
| Vbus | 1kHz | 3 (8) | 3.5ms | 과전압 판정 + 표시 |
| 보드/모터 온도 | 1kHz | 6 (64) | 32ms | 노이즈 제거 |
| 상전류 (표시용) | 20kHz | 5 (32) | 0.8ms | 텔레메트리 |
| 전류 오프셋 보정 | 20kHz | 10 (1024) | 1회성 | `sum >> 10` 평균 |

### 3.2 Fault 관리 (`svc_fault`) — N25 문제 반영

| N25 문제 | N25_STM 대응 |
|----------|--------------|
| nFAULT 미사용 | TIM1_BKIN → **HW가 ns 단위로 PWM 차단** + Break 인터럽트 기록 + 1kHz 폴링 이중화 |
| 순시 과전류 검출 비활성 | 20kHz에서 상전류 3샘플(150µs) 연속 초과 시 차단 |
| 0.8초 평균 전류만으로 보호 | 위 + (다음 단계) I²t 모델 |
| Vbus 미측정 | ADC2 + MAF → OV/UV 3ms 판정 |
| 온도 핸들러 미호출 | 1kHz 태스크에서 항상 실행, 모터 NTC 포함 |
| 반복 클리어로 손상 누적 | **60초 내 3회 초과 클리어 → 영구 래치** |
| 연쇄 에러 시 원인 불명 | **최초 발생 Fault ID와 시각 기록** |
| 3번째 전류센서 미사용 | **3상 합(Ia+Ib+Ic) 진단** (지락, 센서 고장, 상 단선) |
| QEI 카운트 유실 감지 불가 | ABZ 누적 vs MU150 절대값 10Hz 비교 |

### 3.3 파라미터 (`svc_param`)
- 표 하나(`PARAM_LIST` X-macro)에 ID, 기본값, 최소, 최대를 정의 → enum, 기본값, 범위검사가 자동 생성된다
- EEPROM 뱅크 A와 B에 번갈아 저장 + CRC16 + 시퀀스 번호 → 저장 중 전원이 끊겨도 직전 값으로 복구된다
- 새 파라미터는 **목록 맨 끝에만 추가**한다 (저장 슬롯 64개 고정)

### 3.4 CAN 프로토콜 (`svc_comm`) — Node ID 1 기준
| ID | 방향 | 내용 |
|----|------|------|
| 0x601 | Host→Drv | 명령 (byte0 = CMD) |
| 0x581 | Drv→Host | 응답 (byte0 = CMD, byte1 = 결과 0 OK / 1 ARG / 2 STATE / 3 UNKNOWN / 4 HW) |
| 0x181 | Drv→Host | STATUS: fault mask(u32), first fault, state, Vbus[0.1V] |
| 0x281 | Drv→Host | ENC: ENC1 count(i32), ENC2 count(i32) |
| 0x381 | Drv→Host | CUR: Iu, Iv, Iw [10mA] (i16), T_board, T_motor [°C] (i8) |
| 0x481 | Drv→Host | DIAG: ISR 최대 사이클, sched overrun, SPI 에러, ENC 차이 |
| 0x701 | Drv→Host | Heartbeat (state) 1Hz |

데이터는 모두 **Little-endian**이다. 명령 목록은 `svc_comm.h`의 `Comm_Cmd_e`에 있다.

---

## 4. 부팅 시퀀스 (`App_Init`)

```
 1. SVC_Fault_Init / SVC_Sched_Init
 2. BSP_Init
    ├─ DRV_EN=Low, MOE=0 (모든 FET OFF)       ← 가장 먼저
    ├─ SPI CS 모두 High, 브레이크 체결
    ├─ Encoder 타이머 시작
    ├─ ADC 캘리브레이션 → ADC2 DMA 시작, ADC1/3/4 injected arm
    └─ TIM1 시작 (MOE=0 유지) → TRGO2 → 20kHz ADC ISR 동작 시작
 3. 파라미터 로드 (EEPROM A/B, CRC)
 4. Vbus/온도 MAF 초기화
 5. DRV8353: ENABLE → 레지스터 기록 → read-back 검증
 6. MU150 절대값 → ENC1/ENC2 카운터 초기화
 7. 전류 오프셋 보정 (PWM OFF, 1024 샘플)
 8. CAN 시작 (필터: 0x600 + node)
 9. 태스크 등록 → TIM6 시작 → READY (또는 FAULT)
```
부팅 자가진단 실패 항목은 `App_GetBootErrors()` 비트로 남는다.

---

## 5. Bring-up 절차

> 준비: **전류 제한 DC 전원** (처음엔 24V / 1A), CAN 어댑터 (PCAN 등, 1Mbps), 오실로스코프 (게이트 측정은 **차동 프로브**)
> 아래 명령은 `ID: 데이터(hex)` 형식이다. 예: `601: 01` = ID 0x601, 1바이트 0x01

### B0. 첫 전원 / 프로그래밍
| 항목 | 방법 | 통과 기준 |
|------|------|-----------|
| 전원 레일 | 모터 없이 24V 인가, TP에서 12V/5V/3.3V 측정 | 정격 ±5%, 무부하 전류 < 100mA |
| PG 시퀀스 | 12V_PG, 5V_PG LED | 순서대로 점등 |
| SWD 연결 | CubeProgrammer 연결 | MCU ID 읽힘 |
| **Option Byte** | nSWBOOT0=0, nBOOT0=1 | 재전원 시 항상 Flash 부팅 |
| 레벨 확인 (H2) | MU150 A/B 출력 High 레벨 측정 | MCU 입력 허용 전압 이내 (넘으면 bring-up 중단 → HW 대책) |

### B1. 클록 / Heartbeat
| 확인 | 기대값 |
|------|--------|
| LED_A | 1Hz 깜빡임 (READY) / 5Hz (FAULT) |
| CAN 0x701 | 1초마다 1바이트 (1 = READY) |
| MCO 또는 TIM1 PWM 주기 | 20.00kHz (클록 설정 검증) |

### B2. CAN
```
601: 01                      → 581: 01 00 [ver u32] [uid u16]
601: 10                      → 581: 10 00 [active mask u32] [first id] [perm lock]
```
- 첫 부팅이면 `FAULT_WARN_PARAM_DEFAULT`(bit16) 경고가 정상이다

### B3. EEPROM / 파라미터
```
601: 22                      → 기본값 저장 (OK=00)
601: 21 0A 50 46 00 00       → UV 임계(ID 10) = 18000mV (24V 시험용)
601: 22                      → 저장
601: 7F A5                   → 리셋
601: 20 0A                   → 581: 20 00 0A 50 46 00 00  (유지 확인)
```
| 통과 기준 | 리셋 후 값 유지, 저장 직후 전원을 끊어도 다른 뱅크로 복구 |

파라미터 ID: 0 NODE_ID, 1 DIR, 2 POLE_PAIRS, 3 ELEC_OFFSET, 4 ENC_MISMATCH, 5 CUR_LIMIT, 6 CUR_CONT, 7 OC_TRIP, 8 SUM_TRIP, 9 VBUS_OV, 10 VBUS_UV, 11 T_BOARD, 12 T_MOTOR, 13 FOLLOW_ERR, 14 VDS_LVL, 15 IDRIVE_HS, 16 IDRIVE_LS, 17 CAN_TIMEOUT, 18~22 제어 게인(다음 단계)

### B4. Vbus / 온도
```
601: 70 05 64 00             → STATUS + CUR 스트림, 100ms
```
| 확인 | 통과 기준 |
|------|-----------|
| Vbus (0x181 byte6~7) | DMM 대비 ±2% |
| 보드 온도 | 온도계 대비 ±3°C |
| 모터 온도 | **37°C 이하에서는 포화(H1)** — HW 수정 전까지 정상 |
| OV/UV | 전원 전압을 UV 이하로 낮추면 FAULT bit4 |

### B5. DRV8353
```
601: 30 05                   → OCP Control 읽기 (설정값과 일치)
601: 30 06                   → CSA Control (gain 코드 확인)
601: 30 00                   → Fault Status 1 = 0
```
| 통과 기준 | 레지스터 read-back 일치, nFAULT High (FAULT LED 꺼짐) |

### B6. 엔코더
```
601: 40 00                   → MU1 절대 위치 (pos u32, err/warn)
601: 40 01                   → MU2
601: 70 0A 0A 00             → ENC + DIAG 스트림, 10ms
```
| 시험 | 통과 기준 |
|------|-----------|
| 손으로 1회전 | ENC1 변화량 = **±16384** |
| 방향 | ENC1 증가 방향 = MU1 절대값 증가 방향 (반대면 TIM2 IC Polarity 변경) |
| 정합성 활성화 | `601: 21 04 28 00 00 00` (40cnt) → 손으로 여러 번 회전 후 DIAG 차이 < 10 |
| nERR/nWARN | 0 (자석 정상) |

### B7. 전류 오프셋 / 노이즈
```
601: 54                      → 581: 54 00 [off_u u16][off_v][off_w]
```
| 통과 기준 | 오프셋 2048 ± 150, PWM OFF 상태의 CUR 스트림 \|I\| < 0.2A |

### B8. PWM 파형 (모터 미연결)
```
601: 50 01                   → PWM_FIXED 모드 (50%)
601: 51 F4 01 F4 01 F4 01    → U=V=W=500 permil
601: 51 58 02 F4 01 90 01    → U=600, V=500, W=400
601: 50 00                   → OFF
```
| 확인 | 통과 기준 |
|------|-----------|
| 주파수 | 20kHz, center-aligned |
| 데드타임 (GHx↔GLx) | ≥ 300ns, 동시 ON 없음 |
| 상 출력 (SHx) | 듀티대로, 링잉 < Vbus×20% |
| 자동 OFF | 10초 후 / 명령 2초 중단 시 OFF |
| 범위 제한 | 500±150 밖은 거부 (결과 01) |

### B9. Fault 주입
| 시험 | 방법 | 통과 기준 |
|------|------|-----------|
| nFAULT | PWM 중 nFAULT를 GND로 (10k 경유 테스트 클립) | **PWM 즉시 정지 (스코프 < 1µs)**, bit0 set, first_id = 0 |
| 클리어 | `601: 11` | 원인 제거 후에만 OK |
| 클리어 제한 | 60초 내 4회 클리어 | perm_lock = 1, 이후 클리어 불가 |
| UV/OV | 전원 전압 조절 | 해당 비트 set, PWM 정지 |
| WDG | (디버그 빌드) 태스크에 무한루프 삽입 | 250ms 후 리셋, WARN_WDG_RESET |

### B10. 모터 연결 (저전압 24V, 전원 전류 제한 3A)
```
601: 60 01                   → 브레이크 해제
601: 50 02                   → VECTOR 모드
601: 52 32 00 00 00          → 크기 50‰(5%), 전기각 0°  → 회전자 정렬
601: 52 32 00 3C 00          → 60°   (전기각 60° 단위로 360°까지)
601: 50 00
601: 50 03                   → OPENLOOP 모드
601: 53 32 00 32 00          → 크기 50‰, 5.0Hz 전기 → 기계 약 43rpm
601: 53 32 00 CE FF          → −5.0Hz (역회전)
601: 50 00 / 601: 60 00
```
| 확인 | 통과 기준 |
|------|-----------|
| 정렬 (0°) | Iu > 0, Iv ≈ Iw ≈ −Iu/2 → **전류 부호 확인** (반대면 `ADC_RawToMilliAmp` 부호 반전) |
| 60°e 스텝 | ENC1 변화 ≈ **390 cnt** (= 16384 / 7 / 6) → 극쌍수 7 확인 |
| 전기각 오프셋 | 0°e 정렬 시 ENC1 값 → `PARAM_ENC_ELEC_OFFSET_CNT`(ID 3)에 저장 |
| 오픈루프 방향 | +주파수에서 ENC1 증가 (반대면 상순서/`MOTOR_DIR` 정리) |
| 전기 1주기당 ENC 변화 | ≈ 2340.6 cnt |
| 전류 파형 | 정현파, 3상 합 ≈ 0 (FAULT_CUR_SUM 미발생) |
| 온도 | 10초 운전 후 FET/코일 온도 상승 미미 |

**B10까지 통과하면 Phase M(FOC 전류 루프)로 넘어간다.**

---

## 6. 다음 단계 (Phase M) 설계 방향

| 단계 | 구현 | N25 이슈와의 연결 |
|------|------|-------------------|
| M1 | R, L, Ke 측정 기능 (전압 스텝 → 전류 응답) | 게인을 경험값이 아니라 물리값으로 |
| M2 | `ctrl_foc`: Clarke/Park(CORDIC), 전류 PI + **올바른 스케일의 anti-windup**, Vbus 보상 SVPWM, low-side 2상 재구성 | N25 anti-windup 스케일 버그 |
| M3 | `ctrl_speed`: PLL 속도 추정 + 속도 PI | N25에 없던 속도 루프 |
| M4 | `ctrl_pos`: 위치 P + **추종오차 클램프 / 포화 시 기준 적분 정지** | N25 windup → 따라잡기 충격 |
| M5 | `ctrl_i2t` + 온도 디레이팅 + **Vbus 상승 시 회생 토크 제한** | 코일 손상, 링자석 탈락, FET/DRV 소손 |
| M6 | `ctrl_dob`: 부하토크 추정 → 슬립·스톨 감지 → 토크 제한 | 슬립 시 비선형 거동 |
| M7 | AMR 상위 CAN 프로토콜 호환 (N25 메시지 형식) | 기존 시스템 연동 |
| M8 | 실주행 + Fault 주입 + 내구 ([04 문서 §5](04_문제원인_분석_및_개선방안.md) 시험 T1~T7) | 원인 확정 |
