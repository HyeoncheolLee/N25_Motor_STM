# CLAUDE.md — 프로젝트 컨텍스트 (AI 어시스턴트용)

## 사용자 / 커뮤니케이션
- 설명과 문서는 **한국어**로 작성한다.
- 학습용 자료는 `Study/`에 번호를 붙인 md 파일로 남긴다 (`Study/00_README.md` 목차 갱신).
- 진행 상황과 남은 작업은 `TODO.md`에서 관리한다. 작업을 끝내면 체크하고 결과를 기록한다.

## 프로젝트
- AMR 휠 구동 3상 BLDC 드라이버 N25. 기존 PIC32MK 펌웨어(`N25/AMR_MC_N25-PositionCtrl/`, 참고용, 수정하지 않음)에서 **STM32G474 신규 보드**로 전환 중이다.
- 신규 펌웨어: `N25_STM_FW/User/` (CubeIDE + HAL). CubeMX 설정은 `N25_STM_FW/docs/CubeMX_Setup.md`.
- 회로도: `N25/N25_STM_Schematic PDF.pdf` (신규), `N25/N25_Schematic PDF.pdf` (기존).

## 코드 규칙 (반드시 지킬 것)
- 계층: App → Service → Device → BSP → HAL. **include는 아래 방향만**. 위로 알려야 할 때는 콜백 등록이나 `__weak` hook을 쓴다.
- HAL을 직접 호출하는 것은 `Bsp/`뿐이다.
- 파일 `<계층>_<모듈>.c/h` (`bsp_`, `dev_`, `svc_`, `app_`, `util_`, 다음 단계 `ctrl_`).
- 함수 `BSP_PWM_OutputEnable()`, `SVC_Fault_Set()`, `App_BU_SetMode()`, 디바이스는 `DRV8353_ReadReg()` 형태.
- 타입 `_t`(struct) / `_e`(enum), 파일 static 변수 `s_` 접두어, 전역 변수 금지(getter 사용).
- 변수와 매크로에 단위 접미어: `_mA`, `_mV`, `_ms`, `_cnt`, `_x10`, `_permil`, `_Q16`, `_HZ`, `_NS`.
- HW 의존 상수는 `User/Config/board_config.h` 한 곳에만. 확인이 필요한 값은 `TODO(HW)` 주석.
- 센싱 필터는 bit-shift MAF(`util_maf`)를 사용하고, 제어 피드백에는 쓰지 않는다.
- 20kHz ADC ISR(Fast Loop)에서 SPI/EEPROM/CAN 송신/블로킹 금지. SPI는 main loop 태스크에서만.
- 모든 보호는 `SVC_Fault_Set()`을 거친다. Critical이면 즉시 `BSP_PWM_OutputDisable()`.
- 새 파라미터는 `svc_param.h`의 `PARAM_LIST` **맨 끝에만** 추가한다.

## 현재 상태 / 주의
- `N25_STM_FW/User/` 코드는 **아직 한 번도 빌드되지 않았다** (작성 PC에 ARM 툴체인 없음). 빌드 결과를 확인하기 전에는 "동작한다"고 말하지 않는다.
- 회로 리뷰 미해결 항목(H1~H4)은 `Study/06`과 `TODO.md` §2에 있다.
- N25 고장 원인 가설: 위치 기준 windup + 포화 상태 장시간 유지 + 느린/누락된 보호 + 회생 과전압 (`Study/04`). PI 선형성 자체가 1차 원인은 아니라는 결론이다.
