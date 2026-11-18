/**
 * @file    fw_version.h
 * @brief   펌웨어 버전. 형식: MAJOR.MINOR.PATCH + 빌드 날짜
 *          MAJOR: 프로토콜/파라미터 구조 호환성 깨짐
 *          MINOR: 기능 추가
 *          PATCH: 버그 수정
 */
#ifndef FW_VERSION_H
#define FW_VERSION_H

#define FW_VERSION_MAJOR    (0U)
#define FW_VERSION_MINOR    (1U)
#define FW_VERSION_PATCH    (0U)    /* 0.1.0 = Bring-up */

#define FW_VERSION_U32      ((FW_VERSION_MAJOR << 16) | (FW_VERSION_MINOR << 8) | FW_VERSION_PATCH)

#endif /* FW_VERSION_H */
