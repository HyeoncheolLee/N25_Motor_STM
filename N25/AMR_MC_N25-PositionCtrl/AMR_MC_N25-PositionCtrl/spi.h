#ifndef __SPI_H__
#define __SPI_H__

#include "board_definition.h"

// EEPROM Chip Select
#define     EEPROM_SELECT       LATEbits.LATE12 = 0;
#define     EEPROM_DESELECT     LATEbits.LATE12 = 1;

#define     AS5047_SELECT       LATBbits.LATB9 = 0;
#define     AS5047_DESELECT     LATBbits.LATB9 = 1;

#define     MU150_SELECT        LATBbits.LATB5 = 0;
#define     MU150_DESELECT      LATBbits.LATB5 = 1;

#define     MU150_SELECT2       LATFbits.LATF0 = 0;
#define     MU150_DESELECT2     LATFbits.LATF0 = 1;

//#define     ACC_SELECT          LATEbits.LATE14 = 0;
//#define     ACC_DESELECT        LATEbits.LATE14 = 1;

extern void InitSPI(unsigned int data_bit, unsigned int mode);

extern void EEPROM_WRITE(unsigned int ch, unsigned int type, unsigned int data);
extern unsigned int EEPROM_READ(unsigned int ch, unsigned int type);


extern int Push_EEPROM_Ring(unsigned int ch, unsigned int type, unsigned int data);
extern int Handler_EEPROM_Ring();

extern void Init_MU_Ring();
extern int Push_MU_Ring(unsigned int addr, unsigned int data);
extern int Handler_MU_Ring();

extern void InitPVL();

extern void MU_EEPROM_SETTING_BANK_0();
extern void MU_EEPROM_SETTING_BANK_1();
extern int MU_EEPROM_CHECK_BANK_0();
extern int MU_EEPROM_CHECK_BANK_1();


extern int bank_0_index;
extern int bank_1_index;
extern int bank_1_cnt;


extern short MULTI_TURN_DATA;


#define EE_TYPE_MAX                 100

#define EE_COMMON_JMC               0x00
#define EE_COMMON_BOARDTYPE         0x01
#define EE_COMMON_JMC_NOT           0x02
#define EE_COMMON_TEMP_ERROR        0x10
#define EE_COMMON_BOOTLOADER        0x03

#define EE_TYPE_AUTO_SCALE          0x05
#define EE_TYPE_ENC_RES             0x06
#define EE_TYPE_POLE_NUM            0x07
#define EE_TYPE_MOTOR_DIR           0x08


#define EE_TYPE_JAM_LIMIT_CURRENT   0x11
#define EE_TYPE_OVR_LIMIT_CURRENT   0x12
#define EE_TYPE_JAM_LIMIT_TIME      0x13
#define EE_TYPE_OVR_LIMIT_TIME      0x14
#define EE_TYPE_INPUT_ERROR         0x15
#define EE_TYPE_BIG_ERROR           0x16
#define EE_TYPE_POS_LOWER_LIMIT     0x17
#define EE_TYPE_POS_UPPER_LIMIT     0x18


#define EE_TYPE_FOC_PGAIN           0x24
#define EE_TYPE_FOC_IGAIN           0x25
#define EE_TYPE_POS_PGAIN           0x26
#define EE_TYPE_POS_IGAIN           0x27
#define EE_TYPE_POS_DGAIN           0x28

#define EE_TYPE_MU_DISK_OFFSET      0x32
#define EE_TYPE_HOME_ZERO_OFFSET    0x33


// Gain Scheduling
#define EE_TYPE_POS_PGAIN2          0x40
#define EE_TYPE_POS_IGAIN2          0x41
#define EE_TYPE_POS_DGAIN2          0x42

#define EE_TYPE_POS_PGAIN3          0x43
#define EE_TYPE_POS_IGAIN3          0x44
#define EE_TYPE_POS_DGAIN3          0x45

#define EE_TYPE_INIT_DQ_MU_POS      0x50
#define EE_TYPE_INIT_DQ_MU_POS2     0x51

#define EE_TYPE_INIT_DQ_MU_POS_SEL1 0x52
#define EE_TYPE_INIT_DQ_MU_POS_SEL2 0x53

#define EE_TYPE_CURRENT_NULL_RZ     0x60
#define EE_TYPE_CURRENT_NULL_SZ     0x61
#define EE_TYPE_CURRENT_NULL_TZ     0x62



extern unsigned int RomData[MAX_CH+1][EE_TYPE_MAX];



#endif