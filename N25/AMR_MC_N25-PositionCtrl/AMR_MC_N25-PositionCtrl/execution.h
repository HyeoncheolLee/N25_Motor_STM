#ifndef __EXECUTION_H__
#define __EXECUTION_H__

#include "board_definition.h"

#define REQ_NAME_INFO		0x01	// Request Information of Motor Name
#define CMD_REQ_STATUS		0x02	// Request Motor Status
#define CMD_REQ_ENCODER		0x03	// Send Encoder Value			
#define CMD_REQ_CURRENT		0x04	// Send Current Value			
#define CMD_ENC_ZERO		0x06	// Set encoder zero		
#define CMD_INIT_ROM        0x08    // Init EEPROM
#define CMD_FET_ENABLE		0x0B	//				
#define CMD_RUN_CLOSEDLOOP	0x0E	//  Run feedback control loop
#define CMD_CLEAR_ERR		0x75
#define REQ_ROM_DATA        0x24
#define CMD_SET_POS_LOW_LIM 0x50
#define CMD_SET_POS_UP_LIM  0x56
#define SEND_TEMPERATURE    0xA1
#define SET_BOARD_NUM       0xF0
#define SET_CURRENT_LIM     0xF2
#define SET_ERROR_LIM       0xF3
#define RESTORE_DEFAULT     0xFA

#define CMD_SOLENOID_CONTROL        0xC0
#define CMD_CHECK_UVW               0xC1
#define CMD_DQ_ALIGN                0xC2
#define CMD_INIT_CONTROL            0xC3
#define CMD_FRICTION_COMPENSATION   0xC4
#define CMD_MU_DISK_OFFSET          0xC5
#define CMD_HOME_ZERO_OFFSET        0xC6
#define CMD_ENC_RES_DIR             0xC7
#define CMD_JUST_BRAKE              0xC8    // 솔레노이드만 잠금/해제 (모터 제어는 그대로 유지)


#define CMD_FOC_CURRENT_CONTROL     0xD0
#define CMD_FOC_GAIN                0xD2
#define CMD_FOC_OPENLOOP_CONTROL    0xD3
#define CMD_POS_GAIN                0xD5
#define CMD_CURRENT_NULLING         0xD6

#define CMD_SINE_WAVE_TEST          0xE1

#define CMD_MU150_CMD_REG           0xEE
#define CMD_BOOTLOADER_MODE         0xDF



extern int ERROR_FLAG;

extern int InitControlFlag;
extern int InitControlState;
extern int InitControlMode;


extern int sine_wave_amp;
extern int sine_wave_period;
extern int sine_wave_start_pos;
extern int sine_wave_test_cnt;
extern int sine_wave_test_start;

extern int speed_test_target;
extern int speed_test_acc;
extern int speed_test_start_pos;
extern int speed_test_cur_speed;
extern int speed_test_start;




extern int mu_req_eeprom_flag;
extern int mu_req_eeprom_cnt;

extern int mu_send_eeprom_flag;
extern int mu_send_eeprom_cnt;

extern int mu_setting_flag;
extern int mu_setting_stat;
extern int mu_check_flag;
extern int mu_check_stat;



extern int InitPos_DQ_MU;
extern int PRE_PULSE_FLAG;
extern int GO_POS_CNT;
extern int GO_CUR_CNT;
extern int REF_RCV_CNT;
extern int ENC_SHOT_CNT;
extern int REF_MSG_CNT;


extern int MU_CMD_FLAG;
extern int MU_CMD_ADDR;
extern int MU_CMD_DATA;


extern int PRESET_AND_NEW_HOME_POS_FLAG;

extern int SAVE_MU_EEPROM_FLAG;

extern float feedback_gain;
extern float feedforward_gain;

extern int INPUT_DATA_ABS;
extern unsigned int LIMIT_VALID_CNT;
extern int LIMIT_S1;
extern int LIMIT_S2;
extern int LIMIT_S3;
extern int LIMIT_S4;


extern int SAFETY_DATA_ONOFF;
extern int SS1_TARGET_CNT;
extern int SS1_CNT;
extern int SS1_VEL;

extern long long REMOVE_CURRENT_REF_MUL;


extern void GateDriver_OFF();
extern vGateDriver_ON1_ON();
extern float amp2duty(float input, char reverse);
extern void Initialize_MotorParameter();
extern void CMD_ClearErrors();
extern void CMD_SendEncoder();
extern void CMD_SendEncoder_Huboi();
extern void CMD_SendCurrent();


#endif