# AMR_MC_N25 Motor Control Firmware - 프로젝트 구조 분석

## 1. 프로젝트 개요

| 항목 | 내용 |
|------|------|
| 프로젝트명 | AMR_MC_N25 |
| 프로세서 | PIC32MK1024MCF064 (32-bit MIPS) |
| 클럭 | 60 MHz (외부 오실레이터 + SPLL) |
| 빌드 환경 | Microchip MPLAB (NetBeans), XC32 컴파일러 |
| 용도 | 3상 BLDC 모터 FOC 제어 (로봇/AMR용 서보 드라이버) |
| 버전 | 1250621 (AMR - Speed Control) |

---

## 2. 디렉토리 구조

```
AMR_MC_N25.X/
├── main.c                  # 메인 진입점
├── execution.c/h           # CAN 명령 처리 및 상태 머신
├── FOC.c/h                 # Field-Oriented Control (전류 제어)
├── SV_table.c/h            # Space Vector PWM 룩업 테이블
├── can.c/h                 # CAN 버스 드라이버
├── spi.c/h                 # SPI/EEPROM/MU150 센서
├── adc.c/h                 # ADC (전류/온도 센싱)
├── qei.c/h                 # 엔코더 인터페이스 (QEI)
├── control.c/h             # 위치/속도 제어 루프 (PID)
├── pwm.c/h                 # 3상 PWM 출력
├── init.c/h                # 보드 초기화 및 EEPROM 설정
├── status.c/h              # 에러/상태 모니터링
├── home.c/h                # 홈 포지션 이동
├── timer.c/h               # 타이머 설정
├── port.c/h                # GPIO 포트 설정
├── uart.c/h                # UART (최소 구현)
├── dac.c/h                 # DAC (플레이스홀더)
├── board_definition.h      # 보드 상수 정의
├── cpu_config.h            # CPU 설정 pragma
├── app_mk.ld               # 링커 스크립트
├── SVPWM.m                 # MATLAB 참조 파일
├── Makefile                # 빌드 스크립트
├── nbproject/              # MPLAB 프로젝트 설정
├── debug/                  # 디버그 빌드 출력
├── docs/                   # 문서
└── *.pdf                   # 하드웨어 도면 (JMC, CR 드라이버)
```

---

## 3. 소스 파일 상세

### 3.1 핵심 제어 파일

| 파일 | 라인 수 | 역할 |
|------|---------|------|
| **main.c** | 904 | 메인 루프, 하드웨어 초기화, CAN/타이머 핸들링 |
| **execution.c** | 1,535 | CAN 명령 파싱/실행, 모터 제어 모드 관리 |
| **FOC.c** | 846 | FOC 전류 제어 (Iq/Id), Clarke/Park 변환 |
| **control.c** | 243 | 위치/속도 PID 제어 (차동/절대 위치) |
| **home.c** | 140 | 홈 위치 복귀 트라젝토리 |

### 3.2 드라이버 파일

| 파일 | 라인 수 | 역할 |
|------|---------|------|
| **can.c** | 427 | CAN 송수신, 필터링, 링 버퍼 |
| **spi.c** | 783 | SPI 통신, EEPROM R/W, MU150 센서 |
| **adc.c** | 283 | 12-bit ADC, 전류/온도 변환 |
| **qei.c** | 280 | 엔코더 위치/속도 읽기 |
| **pwm.c** | 150 | 3상 PWM (80kHz), 솔레노이드 제어 |
| **timer.c** | 96 | 타이머 초기화 (1kHz 제어, 20kHz 샘플링) |

### 3.3 설정/유틸 파일

| 파일 | 라인 수 | 역할 |
|------|---------|------|
| **init.c** | 317 | EEPROM 파라미터 로드, 보드 식별 |
| **status.c** | 139 | 온도/전류/잼 에러 감시 |
| **SV_table.c** | 738 | sin/cos/tan/atan 룩업 테이블 |
| **board_definition.h** | 143 | 폴 수(7), 전류 상수, 버전 정보 |
| **cpu_config.h** | 168 | CPU pragma 설정 |

---

## 4. 시스템 아키텍처

### 4.1 메인 루프 흐름 (main.c)

```
초기화 시퀀스:
1. CPU 캐시/프리페치 활성화
2. 안정화 대기 (~200ms)
3. 멀티벡터 인터럽트 활성화
4. 아날로그 포트 → 디지털 변환
5. 보드 식별 + 주변장치 초기화
6. 게이트 드라이버 OFF
7. 인터럽트 활성화
8. 엔코더 리셋 (MU150)
9. NON_CTR 상태 체크

메인 루프 (while(1)):
├── CAN TX 링 버퍼 처리
├── 상태 핸들러
├── 에러 체크
├── 제어 루프 실행 (속도/위치)
└── 타이머 카운터 증가
```

### 4.2 인터럽트 구조

```
Timer 2 (1 kHz) ──→ 제어 루프
                     ├── 엔코더 읽기 (QEI)
                     ├── 위치/속도 계산
                     └── 명령 실행 (execution)

Timer 3 (20 kHz) ──→ ADC 샘플링
                     ├── 3상 전류 측정 (R, S, T)
                     ├── FOC 연산 (Iq/Id 제어)
                     ├── SV-PWM 출력
                     └── CAN TX 처리

CAN1 RX ──→ 명령 수신 → 링 버퍼 저장

SPI ──→ MU150 센서 / EEPROM 통신
```

### 4.3 제어 블록 다이어그램

```
CAN 명령 ──→ execution.c (명령 파싱)
                │
                ├── 속도 모드 ──→ control.c (속도 PID)
                ├── 위치 모드 ──→ control.c (위치 PID)
                └── 홈 모드 ──→ home.c (트라젝토리)
                        │
                        ▼
                    FOC.c (전류 제어)
                    ├── Id 제어 (자속)
                    └── Iq 제어 (토크)
                        │
                        ▼
                    SV_table.c (공간벡터 변환)
                        │
                        ▼
                    pwm.c (3상 PWM 출력)
                        │
                        ▼
                    모터 (BLDC 7극쌍)
                        │
                    ┌───┴───┐
                    │       │
                  QEI     ADC
                (위치)   (전류)
```

---

## 5. 주요 하드웨어 파라미터

### 5.1 모터 사양 (board_definition.h)

| 파라미터 | 값 | 설명 |
|----------|-----|------|
| NO_OF_POLE | 7 | 극쌍 수 |
| ENC_POLE_VAL | 2340.571429 | 카운트/극쌍 |
| CURRENT_CONSTANT | 20.1416 | mA/ADC digit (2mΩ 션트 기준) |
| PWM 주파수 | 80 kHz | 3상 에지 정렬 |
| ADC 해상도 | 12-bit | |

### 5.2 타이머 설정

| 타이머 | 주파수 | 용도 |
|--------|--------|------|
| Timer 2 | 1 kHz | 제어 루프 (PR2 = 60000) |
| Timer 3 | 20 kHz | ADC 샘플링/FOC (PR3 = 3000) |
| Timer 4 | 32-bit | 정밀 카운터 |

### 5.3 통신 인터페이스

| 인터페이스 | 용도 |
|-----------|------|
| CAN1 | 상위 제어기 명령/피드백 |
| SPI | MU150 자기 센서 + EEPROM |
| QEI | 엔코더 (모터 + 절대위치) |
| UART | (최소 구현, 디버그용) |

---

## 6. FOC (Field-Oriented Control) 구조

### 6.1 제어 루프 (FOC.c, 20kHz)

```
ADC (Ir, Is, It)
    │
    ▼
Clarke 변환 (3상 → αβ)
    │
    ▼
Park 변환 (αβ → dq, 엔코더 각도 사용)
    │
    ├── Id (자속 전류) → PI 제어 → Vd
    └── Iq (토크 전류) → PI 제어 → Vq
                                    │
                                    ▼
                            역 Park 변환 (dq → αβ)
                                    │
                                    ▼
                            SV-PWM (αβ → 3상 듀티)
                                    │
                                    ▼
                            PWM 레지스터 업데이트
```

### 6.2 주요 기능

- **전류 영점 보정** (Current Nulling): ADC 오프셋 자동 캘리브레이션
- **마찰 보상** (Friction Compensation): 저속 토크 보상
- **오토튜닝** (AutoTuning): PID 게인 자동 조정
- **전류 제한**: 과전류 시 PWM 제한

---

## 7. CAN 통신 프로토콜

### 7.1 명령 구조 (execution.c)

주요 명령 코드 범위:

| 명령 범위 | 용도 |
|----------|------|
| 0x00-0x0F | 상태 요청/응답 |
| 0x10-0x1F | 모터 제어 (속도/위치) |
| 0x20-0x2F | FOC 파라미터 설정 |
| 0x30-0x3F | 엔코더 관련 |
| 0x40-0x4F | EEPROM R/W |
| 0x50+ | 특수 명령 (홈, 테스트 등) |

### 7.2 보드 주소

- JMC (보드 ID)로 CAN 필터 설정
- 다중 모터 제어기 지원

---

## 8. EEPROM 파라미터 (spi.h)

| 타입 코드 | 파라미터 |
|-----------|---------|
| 0x00-0x0F | PID 게인 (Kp, Ki, Kd) |
| 0x10-0x1F | 전류 제한, 오프셋 |
| 0x20-0x2F | 엔코더 설정, 폴 수 |
| 0x30-0x3F | 통신 설정 |
| 0x40-0x4F | 캘리브레이션 데이터 |
| 0x50-0x62 | 모터 특성 파라미터 |

듀얼 뱅크 (Bank 0/1) 지원으로 설정 이중화

---

## 9. 에러 감시 (status.c/h)

| 에러 | 감지 조건 | 동작 |
|------|----------|------|
| 잼 (Jam) | 전류 > 임계값 && 시간 > 제한 | 모터 정지 |
| 과전류 | 순시 전류 초과 | PWM 차단 |
| 보드 온도 | 온도 센서 임계값 | 경고/정지 |
| 모터 온도 | 온도 센서 임계값 | 경고/정지 |
| 엔코더 에러 | 신호 이상 | 에러 플래그 |
| 입력 에러 | CAN 통신 타임아웃 | 안전 정지 |

---

## 10. 참고 문서 (PDF)

| 파일 | 내용 |
|------|------|
| JMC1CH25FLEGSDRV-02.pdf | JMC 모터 드라이버 회로도 Rev.02 |
| JMC1CH25FLEGSDRV-03.pdf | JMC 모터 드라이버 회로도 Rev.03 |
| JMC1CH25FLEGSDRV-5.1.pdf | JMC 모터 드라이버 회로도 Rev.5.1 |
| JMC1CH25FLEGSDRV-8.00.pdf | JMC 모터 드라이버 회로도 Rev.8.00 |
| CR-155W-1CH-DRV-02.pdf | CR 155W 드라이버 회로도 |
| Job1_CR-400W-1CH-DRV-02.PDF | CR 400W 드라이버 회로도 |




