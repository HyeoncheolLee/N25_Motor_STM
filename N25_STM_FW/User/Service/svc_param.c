/**
 * @file    svc_param.c
 */
#include "svc_param.h"
#include "board_config.h"
#include "dev_m95128.h"
#include "util_crc.h"
#include <stddef.h>
#include <string.h>

#define PARAM_MAGIC         (0x4E323550UL)  /* 'N25P' */
#define PARAM_LAYOUT_VER    (1U)            /* 저장 구조 변경 시 증가 */
#define PARAM_CAPACITY      (64U)           /* 저장 슬롯 고정 → 파라미터 추가해도 블록 구조 불변 */

_Static_assert(PARAM_ID_NUM <= PARAM_CAPACITY, "PARAM_LIST exceeds PARAM_CAPACITY");

typedef struct
{
    uint32_t magic;
    uint16_t layout_ver;
    uint16_t count;                 /* 저장 당시 유효 파라미터 수 */
    uint32_t seq;                   /* 저장할 때마다 +1 */
    int32_t  val[PARAM_CAPACITY];
    uint16_t crc;                   /* magic ~ val 까지 CRC16 */
    uint16_t reserved;
} Param_Block_t;

typedef struct
{
    int32_t def;
    int32_t min;
    int32_t max;
} Param_Meta_t;

#define PARAM_META_ITEM(id, d, mn, mx)  { (d), (mn), (mx) },
static const Param_Meta_t s_meta[PARAM_ID_NUM] = { PARAM_LIST(PARAM_META_ITEM) };
#undef PARAM_META_ITEM

static Param_Block_t s_blk;
static uint8_t       s_last_bank = 0U;     /* 0: A, 1: B */
static bool          s_dirty     = false;

static uint16_t Param_CalcCrc(const Param_Block_t *b)
{
    return CRC16_Calc(b, (uint32_t)offsetof(Param_Block_t, crc));
}

static bool Param_IsValid(const Param_Block_t *b)
{
    return (b->magic == PARAM_MAGIC) &&
           (b->layout_ver == PARAM_LAYOUT_VER) &&
           (b->count <= PARAM_CAPACITY) &&
           (b->crc == Param_CalcCrc(b));
}

void SVC_Param_SetDefault(void)
{
    uint32_t seq = s_blk.seq;
    memset(&s_blk, 0, sizeof(s_blk));
    s_blk.magic      = PARAM_MAGIC;
    s_blk.layout_ver = PARAM_LAYOUT_VER;
    s_blk.count      = PARAM_ID_NUM;
    s_blk.seq        = seq;
    for (uint32_t i = 0U; i < PARAM_ID_NUM; i++)
    {
        s_blk.val[i] = s_meta[i].def;
    }
    s_dirty = true;
}

Param_LoadResult_e SVC_Param_Load(void)
{
    static Param_Block_t a, b;      /* 스택 절약 */
    bool ok_a, ok_b, rd_a, rd_b;

    rd_a = EEPROM_Read(EEPROM_ADDR_PARAM_BANK_A, &a, sizeof(a));
    rd_b = EEPROM_Read(EEPROM_ADDR_PARAM_BANK_B, &b, sizeof(b));
    ok_a = rd_a && Param_IsValid(&a);
    ok_b = rd_b && Param_IsValid(&b);

    if (!ok_a && !ok_b)
    {
        s_blk.seq = 0U;
        SVC_Param_SetDefault();
        return (rd_a || rd_b) ? PARAM_LOAD_DEFAULT : PARAM_LOAD_EEPROM_ERR;
    }

    if (ok_a && (!ok_b || (a.seq >= b.seq)))
    {
        s_blk = a;
        s_last_bank = 0U;
    }
    else
    {
        s_blk = b;
        s_last_bank = 1U;
    }

    /* 펌웨어에 새로 추가된 파라미터(count 이후)는 기본값,
     * 저장값이 현재 범위를 벗어나면(범위 변경) 기본값으로 교정 */
    for (uint32_t i = 0U; i < PARAM_ID_NUM; i++)
    {
        if ((i >= s_blk.count) || (s_blk.val[i] < s_meta[i].min) || (s_blk.val[i] > s_meta[i].max))
        {
            s_blk.val[i] = s_meta[i].def;
        }
    }
    s_blk.count = PARAM_ID_NUM;
    s_dirty = false;
    return (s_last_bank == 0U) ? PARAM_LOAD_OK_BANK_A : PARAM_LOAD_OK_BANK_B;
}

bool SVC_Param_Save(void)
{
    static Param_Block_t verify;
    uint8_t  bank = (uint8_t)(s_last_bank ^ 1U);          /* 반대쪽 뱅크에 기록 */
    uint16_t addr = (bank == 0U) ? EEPROM_ADDR_PARAM_BANK_A : EEPROM_ADDR_PARAM_BANK_B;

    s_blk.seq++;
    s_blk.crc = Param_CalcCrc(&s_blk);

    if (!EEPROM_Write(addr, &s_blk, sizeof(s_blk)) ||
        !EEPROM_Read(addr, &verify, sizeof(verify)) ||
        (memcmp(&verify, &s_blk, sizeof(s_blk)) != 0))
    {
        return false;
    }
    s_last_bank = bank;
    s_dirty     = false;
    return true;
}

int32_t SVC_Param_Get(Param_Id_e id)
{
    return (id < PARAM_ID_NUM) ? s_blk.val[id] : 0;
}

bool SVC_Param_Set(Param_Id_e id, int32_t value)
{
    if ((id >= PARAM_ID_NUM) || (value < s_meta[id].min) || (value > s_meta[id].max))
    {
        return false;
    }
    if (s_blk.val[id] != value)
    {
        s_blk.val[id] = value;
        s_dirty = true;
    }
    return true;
}

bool SVC_Param_GetRange(Param_Id_e id, int32_t *min, int32_t *max)
{
    if (id >= PARAM_ID_NUM)
    {
        return false;
    }
    *min = s_meta[id].min;
    *max = s_meta[id].max;
    return true;
}

bool SVC_Param_IsDirty(void)
{
    return s_dirty;
}
