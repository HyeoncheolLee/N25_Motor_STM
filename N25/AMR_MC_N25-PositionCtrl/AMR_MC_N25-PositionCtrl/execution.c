#include <proc/p32mk1024mcf064.h>
#include <sys/attribs.h>
#include <math.h>
#include <string.h>
#include <xc-pic32m.h>


#include "execution.h"
#include "status.h"
#include "can.h"
#include "spi.h"
#include "adc.h"
#include "qei.h"
#include "control.h"
#include "timer.h"
#include "FOC.h"
#include "pwm.h"
#include "home.h"
#include "SV_table.h"



#define ALLMOTORS		0x00
#define	MOTOR0			0x01
#define MOTOR1			0x02



// extern 
int ERROR_FLAG = 0;

int InitControlFlag = 0;
int InitControlState = 0;
int InitControlMode = 0;


int sine_wave_amp = 0;
int sine_wave_period = 0;
int sine_wave_start_pos = 0;
int sine_wave_test_cnt = 0;
int sine_wave_test_start = 0;

int speed_test_target = 0;
int speed_test_acc = 0;
int speed_test_start_pos = 0;
int speed_test_cur_speed = 0;
int speed_test_start = 0;



int mu_req_eeprom_flag = 0;
int mu_req_eeprom_cnt = 0;

int mu_send_eeprom_flag = 0;
int mu_send_eeprom_cnt = 0;

int mu_setting_flag = 0;
int mu_setting_stat = 0;
int mu_check_flag = 0;
int mu_check_stat = 0;




int InitPos_DQ_MU = 0;
int PRE_PULSE_FLAG = 0;
int GO_POS_CNT = 0;
int GO_CUR_CNT = 0;
int REF_RCV_CNT = 0;
int ENC_SHOT_CNT = -1;
int REF_MSG_CNT = 0;


int PRESET_AND_NEW_HOME_POS_FLAG = 0;
int SAVE_MU_EEPROM_FLAG = 0;


int SAFETY_DATA_ONOFF = 1;
int SS1_TARGET_CNT = 0;
int SS1_CNT = 0;
int SS1_VEL = 0;





void GateDriver_OFF();
void GateDriver_ON();
void Initialize_MotorParameter();
void CMD_EncZero(uint8_t data[], uint8_t dlc);
void CMD_InitROM(uint8_t data[], uint8_t dlc);
void CMD_ClearErrors();
void CMD_FetEnable(int onoff);
void CMD_RunClosedLoop(int onoff, int mode);
void CMD_SendEncoder();
void CMD_SendEncoder_Huboi();


static void ExecuteGeneralCMD(uint8_t data[], uint8_t dlc);
static void ExecuteReferenceCMD(uint8_t data[], uint8_t dlc);

static void Send_Temperature();
static void Send_MotorName();
static void CMD_SetPosLowLimit(uint8_t data[], uint8_t dlc);
static void CMD_SetPosUpLimit(uint8_t data[], uint8_t dlc);
static void Set_BOARD(uint8_t data[], uint8_t dlc);
static void Set_Curr_lim(uint8_t data[], uint8_t dlc);
static void Set_Err_lim(uint8_t data[], uint8_t dlc);
static void Send_data1(uint8_t data[], uint8_t dlc);


static void CMD_Solenoid_Control(uint8_t data[], uint8_t dlc);
static void CMD_JustBrake(uint8_t data[], uint8_t dlc);
static void CMD_DQ_Align(uint8_t data[], uint8_t dlc);
static void CMD_InitControl(uint8_t data[], uint8_t dlc);


static void CMD_FOC_Current_Control(uint8_t data[], uint8_t dlc);
static void CMD_FOC_AutoTune_Current(uint8_t data[], uint8_t dlc);
static void CMD_FOC_Gain(uint8_t data[], uint8_t dlc);
static void CMD_FOC_OpenLoop_Control(uint8_t data[], uint8_t dlc);
static void CMD_FOC_AutoTune_Position(uint8_t data[], uint8_t dlc);
static void CMD_POS_Gain(uint8_t data[], uint8_t dlc);
static void CMD_CurrentNulling(uint8_t data[], uint8_t dlc);
static void CMD_SmoothControlOnOff(uint8_t data[], uint8_t dlc);
static void CMD_SineWaveTest(uint8_t data[], uint8_t dlc);
static void CMD_MuDiskOffset(uint8_t data[], uint8_t dlc);
static void CMD_HomeZeroOffset(uint8_t data[], uint8_t dlc);
static void CMD_EncResDir(uint8_t data[], uint8_t dlc);
static void CMD_MU150(uint8_t data[], uint8_t dlc);


//static int prev_tmr4 = 0;
//static int is_first_ref = 1;
static unsigned char t_mode_flag_old = 0;

float feedback_gain = 1.0;
float feedforward_gain = 0.0;


int INPUT_DATA_ABS = 0;
unsigned int LIMIT_VALID_CNT = 10;
int LIMIT_S1 = 0;
int LIMIT_S2 = 0;
int LIMIT_S3 = 0;
int LIMIT_S4 = 0;

long long itemp1_old = 0;

extern unsigned int control_sync_cnt;
extern unsigned int t3_cnt;
static unsigned int last_t3_sync = 0;

static void ExecuteReferenceCMD(uint8_t data[], uint8_t dlc) 
{
    long long itemp1 = 0;
    int itemp2 = 0;
    short stemp;
    
    // receive from mobile
    if(dlc == 8) 
    {
        itemp2 = (int)(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
        stemp = (short)(data[4] | (data[5] << 8));
        NewInputCurrent_f = (float)(stemp);
        
        ENC_SHOT_CNT = 0;
        
        if (CONTROL_MODE == CONTROL_MODE_FOC_POS_CONTROL) 
        {
            NewInputRefDiff = (long long)itemp2;
            GO_POS_CNT = 0;
            GO_CUR_CNT = 0;
            REF_RCV_CNT = 0;
            INPUT_DATA_ABS = 0;
            last_t3_sync = t3_cnt%40;
//            Handler_Control_Diff();
        }
    }
    
    else if(dlc == 3)
    {
        //stop reference seokgyun
        if(data[0] == 0xAB && data[1] == 0xCD && data[2] == 0xEF)
        {
            
            REMOVE_CURRENT_REF_MUL = NewPos * MUL;
//            long long T_REF = NewPos;
//            T_REF = T_REF * MUL;
//            REF_MUL = T_REF;
            uint8_t ackdata[1];
            ackdata[0] = 0xEF;
            Push_CAN_TX_Ring_TMR3(0x44, 1, ackdata);
        }
    }
    else if(dlc == 4)
    {
        itemp2 = (int)(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
        NewInputCurrent_f = 0.0;
        
        ENC_SHOT_CNT = 0;
        
        if (CONTROL_MODE == CONTROL_MODE_FOC_POS_CONTROL) 
        {
            NewInputRefDiff = (long long)itemp2;
            GO_POS_CNT = 0;
            GO_CUR_CNT = 0;
            REF_RCV_CNT = 0;
            INPUT_DATA_ABS = 0;
            last_t3_sync = t3_cnt%40;
//            Handler_Control_Diff();
        
        }
    }
    else if(dlc == 5)
    {
        // for hubo-i only
        itemp2 = data[0] | (data[1] << 8) | (data[2] << 16);
        stemp = data[3] | (data[4] << 8);
        NewInputCurrent = (float)(stemp);
        if (itemp2 & 0x800000) {
            itemp2 |= (0xFF000000);
        }
            
        ENC_SHOT_CNT = 0;
        GO_CUR_CNT = 0;
        
        if (CONTROL_MODE == CONTROL_MODE_FOC_POS_CONTROL) {
            NewInputRefAbs = itemp2;
            GO_POS_CNT = 0;
            GO_CUR_CNT = 0;
            REF_RCV_CNT = 0;
            INPUT_DATA_ABS = 1;
            last_t3_sync = t3_cnt%40;
            Handler_Control_Abs();
        }
    }    
    REF_MSG_CNT = 0;
}

#define MIN_SS1_CNT     20     //50
void __ISR(_CAN1_VECTOR, IPL5AUTO) Handler_CAN_1() {
    static uint8_t message[8];
    static uint32_t can_id;
    static uint8_t dlc;

    //if(CAN_READ(&can_id, message, &dlc)){
    while (CAN_READ(&can_id, message, &dlc) == 1) {
        if (can_id == CANID_REF) {
            ExecuteReferenceCMD(message, dlc);
        } else if (can_id == CANID_GEN) {
            ExecuteGeneralCMD(message, dlc);
        } else if (can_id == CANID_SS1 || can_id == CANID_SS2) {
            int temp_mode = message[0]; // 0:Normal 1:STO  2:SS1  3:SS2
            int temp_time = message[1]; // 10ms cnt
            if(temp_mode == 0){
                // Go back to Normal
                CONTROL_MODE = CONTROL_MODE_FOC_POS_CONTROL;
            }else if(temp_mode == 1){
                // STO
                CONTROL_MODE = CONTROL_MODE_STO;
            }else if(temp_mode == 2){
                // SS1
                switch(CONTROL_MODE){
                case CONTROL_MODE_FOC_POS_CONTROL:                
                    SS1_TARGET_CNT = temp_time*5;   //2ms cnt
                    if(SS1_TARGET_CNT < MIN_SS1_CNT){     // minimum 40ms
                        SS1_TARGET_CNT = MIN_SS1_CNT;
                    }
                    SS1_TARGET_CNT += 20;       // NOTE: TO BE CHECKED
                    SS1_CNT = 0;
                    SS1_VEL = LAST_VELOCITY;
                    
                    control_sync_cnt = last_t3_sync;
//                    CONTROL_MODE = CONTROL_MODE_SS1_POS;  // NOTE: TO BE CHECKED
                    CONTROL_MODE = CONTROL_MODE_SS2_POS;
                    break;
                case CONTROL_MODE_FOC_HOME_CONTROL:
                case CONTROL_MODE_FOC_CUR_CONTROL:
                case CONTROL_MODE_FOC_OPENLOOP_CONTROL:
                case CONTROL_MODE_NO_ACT:
                    CONTROL_MODE = CONTROL_MODE_STO;
                    break;
                }
            }else if(temp_mode == 3){
                // SS2
                switch(CONTROL_MODE){
                case CONTROL_MODE_FOC_POS_CONTROL:                
                    SS1_TARGET_CNT = temp_time*5;   //2ms cnt
                    if(SS1_TARGET_CNT < MIN_SS1_CNT){     // minimum 40ms
                        SS1_TARGET_CNT = MIN_SS1_CNT;
                    }
                    SS1_TARGET_CNT += 20;       // NOTE: TO BE CHECKED
                    SS1_CNT = 0;
                    SS1_VEL = LAST_VELOCITY;
                    
                    control_sync_cnt = last_t3_sync;
                    CONTROL_MODE = CONTROL_MODE_SS2_POS;
                    break;
                case CONTROL_MODE_FOC_HOME_CONTROL:
                case CONTROL_MODE_FOC_CUR_CONTROL:
                case CONTROL_MODE_FOC_OPENLOOP_CONTROL:
                case CONTROL_MODE_NO_ACT:
                    CONTROL_MODE = CONTROL_MODE_STO;
                    break;
                }
            }
        }
    }

    IFS5bits.CAN1IF = 0;
}

static void ExecuteGeneralCMD(uint8_t data[], uint8_t dlc) {
    int itemp;
    int itemp2;

    switch (data[0]) {

        case REQ_NAME_INFO:
            Send_MotorName();
            SET_STATUS_FLAG
            break;

        case CMD_REQ_STATUS: // Request motor status
            SET_STATUS_FLAG
            break;

        case CMD_REQ_ENCODER: // Request Encoder value
            CMD_SendEncoder();
            break;

        case CMD_REQ_CURRENT: // Request Current value
            CMD_SendCurrent();
            break;

        case CMD_ENC_ZERO:
            CMD_EncZero(data, dlc);
            break;
            
        case CMD_INIT_ROM:
            CMD_InitROM(data, dlc);
            break;
            
        case CMD_CLEAR_ERR: // Set Error flag clear
            CMD_ClearErrors();
            SET_STATUS_FLAG
            break;

        case CMD_FET_ENABLE:
            itemp = data[2]&0x01;
            CMD_FetEnable(itemp);
            break;

        case CMD_RUN_CLOSEDLOOP:
            itemp = data[2]&0x01;
            itemp2 = data[3];
            CMD_RunClosedLoop(itemp, itemp2);
            break;

        case REQ_ROM_DATA:
            Send_data1(data, dlc);
            break;

        case CMD_SET_POS_LOW_LIM:
            CMD_SetPosLowLimit(data, dlc);
            break;

        case CMD_SET_POS_UP_LIM:
            CMD_SetPosUpLimit(data, dlc);
            break;

        case SEND_TEMPERATURE:
            Send_Temperature();
            break;

        case SET_BOARD_NUM:
            Set_BOARD(data, dlc);
            break;

        case SET_CURRENT_LIM:
            Set_Curr_lim(data, dlc);
            break;

        case SET_ERROR_LIM:
            Set_Err_lim(data, dlc);
            break;

        case RESTORE_DEFAULT:
            //INIT_BOARD_MODE = INIT_BOARD_MODE_RESORE;
            break;


            // =======================================

        case CMD_SOLENOID_CONTROL:
            CMD_Solenoid_Control(data, dlc);
            break;

        case CMD_JUST_BRAKE:
            CMD_JustBrake(data, dlc);
            break;

        case CMD_DQ_ALIGN:
            CMD_DQ_Align(data, dlc);
            break;

        case CMD_INIT_CONTROL:
            CMD_InitControl(data, dlc);
            break;

            // =======================================

        case CMD_FOC_CURRENT_CONTROL:
            CMD_FOC_Current_Control(data, dlc);
            break;

        case CMD_FOC_GAIN:
            CMD_FOC_Gain(data, dlc);
            break;

        case CMD_FOC_OPENLOOP_CONTROL:
            CMD_FOC_OpenLoop_Control(data, dlc);
            break;

        case CMD_POS_GAIN:
            CMD_POS_Gain(data, dlc);
            break;

        case CMD_CURRENT_NULLING:
            CMD_CurrentNulling(data, dlc);
            break;

      
        case CMD_SINE_WAVE_TEST:
            CMD_SineWaveTest(data, dlc);
            break;

        case CMD_MU_DISK_OFFSET:
            CMD_MuDiskOffset(data, dlc);
            break;

        case CMD_HOME_ZERO_OFFSET:
            CMD_HomeZeroOffset(data, dlc);
            break;

        case CMD_ENC_RES_DIR:
            CMD_EncResDir(data, dlc);
            break;

        case CMD_MU150_CMD_REG:
            CMD_MU150(data, dlc);
            break;
    }
}

void Send_MotorName() {
    char canTxData[8] = {0,};

    canTxData[0] = REQ_NAME_INFO;
    canTxData[1] = BTY_1CH;
    canTxData[2] = 1;
    canTxData[3] = VERSION & 0xFF;
    canTxData[4] = (VERSION >> 8) &0xFF;
    canTxData[5] = (VERSION >> 16)&0xFF;
    canTxData[6] = (VERSION >> 24)&0xFF;

    Push_CAN_TX_Ring(CANID_DATA, 7, canTxData);
}

static int temp_send_toggle = 0;

short send_current_q = 0;
long long ltemp1_old=0;
int send_encoder_counter = 0;

void CMD_SendEncoder() {
    unsigned char canTxData[8] = {0,};
    long long ltemp1 = NewPos + ABS_OFFSET_ENC;
    
    send_current_q = 0;
    if (measured_current_q_avg > 32767)
        send_current_q = 32767;
    else if (measured_current_q_avg < -32767)
        send_current_q = -32767;
    else
        send_current_q = measured_current_q_avg;

    if (MDR == 1) {
        send_current_q = -send_current_q;
    }
    
canTxData[0] = (long long)(ltemp1) & 0xFF;
canTxData[1] = ((long long)(ltemp1) >> 8) & 0xFF;
canTxData[2] = ((long long)(ltemp1) >> 16) & 0xFF;
canTxData[3] = ((long long)(ltemp1) >> 24) & 0xFF;
canTxData[4] = ((long long)(ltemp1) >> 32) & 0xFF;
canTxData[5] = ((long long)(ltemp1) >> 40) & 0xFF;
canTxData[6] = ((long long)(ltemp1) >> 48) & 0xFF;
canTxData[7] = ((long long)(ltemp1) >> 56) & 0xFF;

Push_CAN_TX_Ring_TMR3(CANID_ENC2, 8, canTxData);
    send_encoder_counter++;
}


void CMD_SendEncoder_Huboi(){
    unsigned char canTxData[8] = {0,};
    long long ltemp1 = NewPos + ABS_OFFSET_ENC;
    int send_temp = ltemp1 & 0x007FFFFF;
    if (ltemp1 < 0)  send_temp |= 0x800000;
//    long long send_temp_m = ltemp1 & 0x007FFFFFFFFFFFFF;
//    if (ltemp1 < 0)  send_temp_m |= 0x80000000000000;
    
    send_current_q = 0;
    if (measured_current_q_avg > 32767)
        send_current_q = 32767;
    else if (measured_current_q_avg < -32767)
        send_current_q = -32767;
    else
        send_current_q = measured_current_q_avg;

    if (MDR == 1) {
        send_current_q = -send_current_q;
    }
        
    canTxData[0] = send_temp & 0xFF;
    canTxData[1] = (send_temp >> 8) & 0xFF;
    canTxData[2] = (send_temp >> 16) & 0xFF;
    canTxData[3] = send_current_q & 0xFF;
    canTxData[4] = (send_current_q >> 8) & 0xFF;
    canTxData[5] = ((STAT.b.RUN) | ((STAT.b.MOD) << 1) | ((STAT.b.JAM) << 2) | ((STAT.b.CUR) << 3) | ((STAT.b.BIG) << 4) | ((STAT.b.INP) << 5) | ((STAT.b.PS1 | STAT.b.PS2) << 6) | ((STAT.b.NON_CTR) << 7));
    canTxData[6] = temperature_board_celsius_x1 & 0x7F;        
    canTxData[7] = 0x00;

    Push_CAN_TX_Ring_TMR3(CANID_ENC, 8, canTxData);
}


void CMD_SendCurrent() {
    char canTxData[8] = {0,};
    if (RPM < 0)
        RPM = -RPM;
    int send_rpm = RPM / 10;
    //send_rpm = temperature_chip_celsius_x1*10;
    //send_rpm = temperature_board_celsius_x1;
    
    canTxData[0] = measured_current_q_avg & 0xFF;
    canTxData[1] = (measured_current_q_avg >> 8)&0xFF;
    canTxData[2] = send_rpm & 0xFF;
    canTxData[3] = (send_rpm >> 8)&0xFF;

    Push_CAN_TX_Ring(CANID_DATA, 4, canTxData);
}

void CMD_EncZero(uint8_t data[], uint8_t dlc) {
    int ch = data[1];
    CMD_ClearErrors();
    STAT.b.INIT = 0;
    Initialize_MotorParameter();
    //STAT.B=0;

    CONTROL_MODE = CONTROL_MODE_NO_ACT;
#ifdef NEW_AMP2
    PDC1 = PWM_RES;
    PDC2 = PWM_RES;
    PDC3 = PWM_RES;
#else
    PDC1 = 0;
    PDC2 = 0;
    PDC3 = 0;
#endif

    // solenoid hard push
    sole_hard_push_cnt = SOLENOID_HARD_PUSH_COUNT;
    solenoid_stat = SOLENOID_STAT_HARD_PUSH;


    DQ_align(data[2]);

    SET_STATUS_FLAG
}



void CMD_InitROM(uint8_t data[], uint8_t dlc){
    if(data[1] == 0){
        // EEPROM
        if(data[2] == 0){
            // Init EEPROM
            if((data[3] == 0x77) && (data[4] == 0x89)){ // KEY
                InitROM(1); // push to buffer
            }
        }else if(data[2] == 1){
            // Check EEPROM
            int notok = 0;
            if(JAM_LIM_Current != DEFAULT_JAM_LIMIT_CURRENT){
                notok = 1;
            }
            if(OVR_LIM_Current != DEFAULT_OVR_LIMIT_CURRENT){
                notok = 1;
            }
            if(JAM_LIM_Time != DEFAULT_JAM_LIMIT_TIME){
                notok = 1;
            }
            if(OVR_LIM_Time != DEFAULT_OVR_LIMIT_TIME){
                notok = 1;
            }
            if(I_ERR != DEFAULT_INPUT_ERROR){
                notok = 1;
            }
            if(B_ERR != DEFAULT_BIG_ERROR){
                notok = 1;
            }
            
            unsigned char canTxData[8] = {0,};
            canTxData[0] = CMD_INIT_ROM;
            canTxData[1] = 0;   // eeprom
            if(notok == 0){
                canTxData[2] = 1;   // success
            }else{
                canTxData[2] = 0;   // fail
            }
            Push_CAN_TX_Ring(CANID_DATA, 3, canTxData);
        }
    }else if(data[1] == 1){
//        // LUT
//        if(data[2] == 0){
//            // Init LUT
//            if((data[3] == 0x77) && (data[4] == 0x89)){ // KEY
//                InitLUT(1); // push to buffer
//            }
//        }else if(data[2] == 1){
//            // Check LUT
//        }
    }
}



void CMD_ClearErrors() {
    STAT.B &= 0xFFFF00FF;
    JAM_cnt = 0;
    OVR_cnt = 0;
    ERROR_FLAG = 0;
}

void CMD_FetEnable(int onoff) {
    //UpdateEncoder();
    //ERROR_sum = 0;
    STAT.b.RUN = 0;
    if (onoff == 1) {
        if (STAT.b.MT_ERR == 1) {
            return;
        }
        GateDriver_ON();
    } else {
        GateDriver_OFF();
    }
}

void CMD_RunClosedLoop(int onoff, int mode) {
    // mode 0: original
    // mode 1: gravity <--> position

    if (STAT.b.FET == 1) {
        //ERROR_sum = 0;
        ERROR_sum_pushing = 0;
        if (Igain_pos >= 1) {
            float temp_meas_q = measured_current_q;
            if (MDR == 1) {
                temp_meas_q = -temp_meas_q;
            }
            ERROR_sum = (float) temp_meas_q * 10.0 / (float) Igain_pos;
        } else {
            ERROR_sum = 0;
        }

        if (onoff == 1) {
            Initialize_ControlParameter();
            //UpdateEncoder();
            NewInputRefAbs = REF = PREV_REF = NewPos + ABS_OFFSET_ENC;
            REF_MUL = REF * MUL;
            NewInputRefDiff = 0;
            if (mode == 0) {
                ref_current_q = 0;
                FOC_Init_CurrentControl();
            }
            STAT.b.RUN = 1;
            STAT.b.MOD = CTRL_MOD_POSITION;
            CONTROL_MODE = CONTROL_MODE_FOC_POS_CONTROL;
        } else {
            STAT.b.RUN = 0;

            CONTROL_MODE = CONTROL_MODE_NO_ACT;
#ifdef NEW_AMP2
            PDC1 = PWM_RES;
            PDC2 = PWM_RES;
            PDC3 = PWM_RES;
#else
            PDC1 = 0;
            PDC2 = 0;
            PDC3 = 0;
#endif
        }
    } else {
        STAT.b.RUN = 0;

        CONTROL_MODE = CONTROL_MODE_NO_ACT;
#ifdef NEW_AMP2
        PDC1 = PWM_RES;
        PDC2 = PWM_RES;
        PDC3 = PWM_RES;
#else
        PDC1 = 0;
        PDC2 = 0;
        PDC3 = 0;
#endif
    }
}

void Initialize_MotorParameter() {
    Initialize_ControlParameter();

    currentMotorPosition = 0;
    MAG_AXIS = 0;
    MAG_AXIS_TEMP = 0;
    MAG_AXIS_TEMP_INIT = 0;
    
    REF = 0; // Reference Command
    REF_MUL = 0;
    PREV_REF = 0;

    NewInputRefDiff = 0; // New Reference
    NewInputRefAbs = 0;

    NewPos = 0; // Current Encoder Position
    NewPos2 = 0;
    OldPos = 0; // Previous Encoder Position
    OldPos2 = 0;
}

void GateDriver_ON() {
    if ((STAT.B & 0x00000F00) != 0)
        return;

    //    IOCON1bits.OVRDAT = 0b00;
    //    IOCON2bits.OVRDAT = 0b00;
    //    IOCON3bits.OVRDAT = 0b00;
    //    IOCON1SET = 0x0300;
    //    IOCON2SET = 0x0300;
    //    IOCON3SET = 0x0300;

    
    MOTOR_HIP_ON

    //    IOCON1CLR = 0x0300;
    //    IOCON2CLR = 0x0300;
    //    IOCON3CLR = 0x0300;

    STAT.b.FET = 1;
}

void GateDriver_OFF() {
#ifdef NEW_AMP2
    PDC1 = PWM_RES;
    PDC2 = PWM_RES;
    PDC3 = PWM_RES;
#else
    PDC1 = 0;
    PDC2 = 0;
    PDC3 = 0;
#endif
    MOTOR_HIP_OFF
    STAT.b.FET = 0;
    STAT.b.RUN = 0;
}

void Send_data1(uint8_t data[], uint8_t dlc) {
    unsigned int send_dlc;
    unsigned int itemp;
    int ch, data_type, is_err = 0;
    char canTxData[8] = {0,};

    ch = data[1] - 1;
    data_type = data[2];

    if (ch != 0)
        return;

    switch (data_type) {
        case 1:
            send_dlc = 8;
            canTxData[0] = 0;
            canTxData[1] = 0;
            canTxData[2] = 0;
            canTxData[3] = 0;
            canTxData[4] = 0;
            canTxData[5] = 0;
//            itemp = Encoder_res & 0x3FFF | ((MDR & 0x01) << 15);
            itemp = Encoder_res & 0x3FFF | ((HomeDir & 0x01) << 15);
            canTxData[6] = itemp & 0xFF;
            canTxData[7] = (itemp >> 8)&0xFF;
            break;
        case 2:
            send_dlc = 8;
            canTxData[0] = 0;
            canTxData[1] = 0;
            canTxData[2] = 0;
            canTxData[3] = 0;
            canTxData[4] = 0;
            canTxData[5] = 0;
            canTxData[6] = 0;
            canTxData[7] = 0;
            break;
        case 3:
            send_dlc = 8;
            canTxData[0] = 0;
            canTxData[1] = 0;
            canTxData[2] = RomData[0][EE_TYPE_POS_LOWER_LIMIT] & 0xFF;
            canTxData[3] = (RomData[0][EE_TYPE_POS_LOWER_LIMIT] >> 8)&0xFF;
            canTxData[4] = (RomData[0][EE_TYPE_POS_LOWER_LIMIT] >> 16)&0xFF;
            canTxData[5] = (RomData[0][EE_TYPE_POS_LOWER_LIMIT] >> 24)&0xFF;
            canTxData[6] = RomData[0][EE_TYPE_POS_UPPER_LIMIT] & 0xFF;
            canTxData[7] = (RomData[0][EE_TYPE_POS_UPPER_LIMIT] >> 8)&0xFF;
            break;
        case 4:
            send_dlc = 8;
            canTxData[0] = (RomData[0][EE_TYPE_POS_UPPER_LIMIT] >> 16)&0xFF;
            canTxData[1] = (RomData[0][EE_TYPE_POS_UPPER_LIMIT] >> 24)&0xFF;
            canTxData[2] = 0;
            canTxData[3] = 0;
            canTxData[4] = 0;
            canTxData[5] = 0;
            break;
        case 5:
            send_dlc = 8;
            canTxData[0] = 0;
            canTxData[1] = 0;
            canTxData[2] = 0;
            canTxData[3] = 0;
            canTxData[4] = 0;
            canTxData[5] = 0;
            canTxData[6] = RomData[0][EE_TYPE_JAM_LIMIT_CURRENT] & 0xFF;
            canTxData[7] = (RomData[0][EE_TYPE_JAM_LIMIT_CURRENT] >> 8)&0xFF;
            break;
        case 6:
            send_dlc = 8;
            canTxData[0] = RomData[0][EE_TYPE_OVR_LIMIT_CURRENT] & 0xFF;
            canTxData[1] = (RomData[0][EE_TYPE_OVR_LIMIT_CURRENT] >> 8)&0xFF;
            canTxData[2] = RomData[0][EE_TYPE_JAM_LIMIT_TIME] & 0xFF;
            canTxData[3] = (RomData[0][EE_TYPE_JAM_LIMIT_TIME] >> 8)&0xFF;
            canTxData[4] = RomData[0][EE_TYPE_OVR_LIMIT_TIME] & 0xFF;
            canTxData[5] = (RomData[0][EE_TYPE_OVR_LIMIT_TIME] >> 8)&0xFF;
            canTxData[6] = 0;
            canTxData[7] = 0;
            break;
        case 7:
            send_dlc = 8;
            canTxData[0] = 0;
            canTxData[1] = 0;
            canTxData[2] = 0;
            canTxData[3] = 0;
            canTxData[4] = RomData[0][EE_TYPE_INPUT_ERROR] & 0xFF;
            canTxData[5] = (RomData[0][EE_TYPE_INPUT_ERROR] >> 8)&0xFF;
            canTxData[6] = RomData[0][EE_TYPE_BIG_ERROR] & 0xFF;
            canTxData[7] = (RomData[0][EE_TYPE_BIG_ERROR] >> 8)&0xFF;
            break;
        case 8:
            send_dlc = 6;
            canTxData[0] = RomData[MAX_CH][EE_COMMON_JMC] & 0xFF;
            canTxData[1] = RomData[MAX_CH][EE_COMMON_JMC]&0xFF;
            canTxData[2] = 0;
            canTxData[3] = RomData[MAX_CH][EE_COMMON_BOARDTYPE]&0xFF;
            canTxData[4] = RomData[MAX_CH][EE_COMMON_TEMP_ERROR] & 0xFF;
            canTxData[5] = (RomData[MAX_CH][EE_COMMON_TEMP_ERROR] >> 8)&0xFF;
            break;
        default:
            is_err = 1;
            break;
    }
    if (is_err == 0)
        Push_CAN_TX_Ring(CANID_DATA, send_dlc, canTxData);
}

void CMD_SetPosLowLimit(uint8_t data[], uint8_t dlc) {
    int ch = data[1] - 1;
    if (ch != 0)
        return;

    MAX_POS1_flag = (data[2])&0x01;
    if ((data[2]&0x02) == 0x02) {
        POS_LOWER_LIM = (data[6]);
        POS_LOWER_LIM = (POS_LOWER_LIM << 8) | data[5];
        POS_LOWER_LIM = (POS_LOWER_LIM << 8) | data[4];
        POS_LOWER_LIM = (POS_LOWER_LIM << 8) | data[3];
    }
    if (MAX_POS1_flag == 1)
        POS_LOWER_LIM |= 0x01;
    else
        POS_LOWER_LIM &= 0xFFFFFFFE;

    Push_EEPROM_Ring(0, EE_TYPE_POS_LOWER_LIMIT, POS_LOWER_LIM);
    RomData[0][EE_TYPE_POS_LOWER_LIMIT] = POS_LOWER_LIM;
}

void CMD_SetPosUpLimit(uint8_t data[], uint8_t dlc) {
    int ch = data[1] - 1;
    if (ch != 0)
        return;

    MAX_POS2_flag = (data[2])&0x01;
    if ((data[2]&0x02) == 0x02) {
        POS_UPPER_LIM = (data[6]);
        POS_UPPER_LIM = (POS_UPPER_LIM << 8) | data[5];
        POS_UPPER_LIM = (POS_UPPER_LIM << 8) | data[4];
        POS_UPPER_LIM = (POS_UPPER_LIM << 8) | data[3];
    }
    if (MAX_POS2_flag == 1)
        POS_UPPER_LIM |= 0x01;
    else
        POS_UPPER_LIM &= 0xFFFFFFFE;

    Push_EEPROM_Ring(0, EE_TYPE_POS_UPPER_LIMIT, POS_UPPER_LIM);
    RomData[0][EE_TYPE_POS_UPPER_LIMIT] = POS_UPPER_LIM;
}

void Set_BOARD(uint8_t data[], uint8_t dlc) {
    int tempJMC, tempBTY;
    tempJMC = data[1]; // | (1<<8);
    tempBTY = data[2];

    if (JMC == 0 || JMC == 2 || JMC == 4) {
        if (tempJMC == 0 || JMC == 2 || JMC == 4) {
            // direction not change
        } else {
            // direction change
            HOME_ZERO_OFFSET = -HOME_ZERO_OFFSET;
            Push_EEPROM_Ring(0, EE_TYPE_HOME_ZERO_OFFSET, HOME_ZERO_OFFSET);
        }
    } else {
        if (tempJMC == 0 || JMC == 2 || JMC == 4) {
            // direction change
            HOME_ZERO_OFFSET = -HOME_ZERO_OFFSET;
            Push_EEPROM_Ring(0, EE_TYPE_HOME_ZERO_OFFSET, HOME_ZERO_OFFSET);
        } else {
            // direction not change
        }
    }

    Push_EEPROM_Ring(MAX_CH, EE_COMMON_JMC, tempJMC);
    Push_EEPROM_Ring(MAX_CH, EE_COMMON_BOARDTYPE, tempBTY);
    RomData[MAX_CH][EE_COMMON_JMC] = tempJMC;
    RomData[MAX_CH][EE_COMMON_BOARDTYPE] = tempBTY;
}

void Send_Temperature() {
    unsigned char canTxData[8] = {0,};

    canTxData[0] = SEND_TEMPERATURE;
    canTxData[1] = temperature_board_celsius_x10 & 0xFF;
    canTxData[2] = (temperature_board_celsius_x10 >> 8) &0xFF;
    canTxData[3] = temperature_motor_celsius_x10 & 0xFF;
    canTxData[4] = (temperature_motor_celsius_x10 >> 8)&0xFF;
    canTxData[5] = 0 & 0xFF;
    canTxData[6] = (0 >> 8)&0xFF;

    Push_CAN_TX_Ring(CANID_DATA, 7, canTxData);
}

void Set_Curr_lim(uint8_t data[], uint8_t dlc) {
    int ch = data[1] - 1;

    if (ch != 0)
        return;

    JAM_LIM_Current = (short) (data[2] | (data[3] << 8));
    OVR_LIM_Current = (short) (data[4] | (data[5] << 8));
    JAM_LIM_Time = (int) ((unsigned char) data[6]);
    OVR_LIM_Time = (int) ((unsigned char) data[7]);

    Push_EEPROM_Ring(0, EE_TYPE_JAM_LIMIT_CURRENT, JAM_LIM_Current);
    Push_EEPROM_Ring(0, EE_TYPE_OVR_LIMIT_CURRENT, OVR_LIM_Current);
    Push_EEPROM_Ring(0, EE_TYPE_JAM_LIMIT_TIME, JAM_LIM_Time);
    Push_EEPROM_Ring(0, EE_TYPE_OVR_LIMIT_TIME, OVR_LIM_Time);
    RomData[0][EE_TYPE_JAM_LIMIT_CURRENT] = JAM_LIM_Current;
    RomData[0][EE_TYPE_OVR_LIMIT_CURRENT] = OVR_LIM_Current;
    RomData[0][EE_TYPE_JAM_LIMIT_TIME] = JAM_LIM_Time;
    RomData[0][EE_TYPE_OVR_LIMIT_TIME] = OVR_LIM_Time;
}

void Set_Err_lim(uint8_t data[], uint8_t dlc) {
    unsigned int i, b, e;
    int ch = data[1] - 1;

    if (ch != 0)
        return;

    i = (data[3]);
    i = (i << 8) | data[2];
    b = (data[5]);
    b = (b << 8) | data[4];
    e = (data[7]);
    e = (e << 8) | data[6];
    I_ERR = (long) i;
    B_ERR = (long) b;
    T_ERR = e;

    if (I_ERR > 0xFFFF)
        I_ERR = 0xFFFF;

    if (B_ERR > 0xFFFF)
        B_ERR = 0xFFFF;

    Push_EEPROM_Ring(0, EE_TYPE_BIG_ERROR, B_ERR);
    Push_EEPROM_Ring(0, EE_TYPE_INPUT_ERROR, I_ERR);
    Push_EEPROM_Ring(MAX_CH, EE_COMMON_TEMP_ERROR, T_ERR);
    RomData[0][EE_TYPE_BIG_ERROR] = B_ERR;
    RomData[0][EE_TYPE_INPUT_ERROR] = I_ERR;
    RomData[0][EE_COMMON_TEMP_ERROR] = T_ERR;
}

void CMD_Solenoid_Control(uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        // turn off
        solenoid_stat = SOLENOID_STAT_TURN_OFF;

        STAT.b.RUN = 0;

        CONTROL_MODE = CONTROL_MODE_NO_ACT;
#ifdef NEW_AMP2
        PDC1 = PWM_RES;
        PDC2 = PWM_RES;
        PDC3 = PWM_RES;
#else
        PDC1 = 0;
        PDC2 = 0;
        PDC3 = 0;
#endif

        GateDriver_OFF();

    } else if (data[1] == 1) {
        // hard push
        sole_hard_push_cnt = SOLENOID_HARD_PUSH_COUNT;
        solenoid_stat = SOLENOID_STAT_HARD_PUSH;
    } else if (data[1] == 2) {
        // weak push
        solenoid_stat = SOLENOID_STAT_WEAK_PUSH;
    }
}

// justbrake: 솔레노이드(브레이크)만 잠그거나 해제. 모터 제어는 그대로 유지.
//   CMD_Solenoid_Control 과 달리 STAT.b.RUN / CONTROL_MODE / PWM / GateDriver 를
//   건드리지 않으므로 FOC closed-loop 가 계속 동작한다.
//   사용 케이스: SS2 logic 처럼 모터 제어를 유지하면서 브레이크만 잠그고 싶을 때.
//
//   data[0] = CMD_JUST_BRAKE (0xC8)
//   data[1] = 0 → Lock  (솔레노이드 OFF → 스프링이 브레이크 잠금)
//             1 → Release (솔레노이드 HARD PUSH → 브레이크 해제)

void CMD_JustBrake(uint8_t data[], uint8_t dlc) {
    if (dlc < 2) {
        return;
    }
    if (data[1] == 0) {
        // Lock — 솔레노이드 OFF (스프링 자동 잠금) 만
        solenoid_stat = SOLENOID_STAT_TURN_OFF;
    } else if (data[1] == 1) {
        // Release — 솔레노이드 HARD PUSH (브레이크 해제)
        sole_hard_push_cnt = SOLENOID_HARD_PUSH_COUNT;
        solenoid_stat = SOLENOID_STAT_HARD_PUSH;
    }
}

void CMD_DQ_Align(uint8_t data[], uint8_t dlc) {
    CMD_ClearErrors();

    DQ_align(data[1]);
}


void CMD_InitControl(uint8_t data[], uint8_t dlc) {
    InitControlState = 1;
    InitControlFlag = 1;
}


void CMD_FOC_Current_Control(uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        // stop
        CONTROL_MODE = CONTROL_MODE_NO_ACT;
#ifdef NEW_AMP2
        PDC1 = PWM_RES;
        PDC2 = PWM_RES;
        PDC3 = PWM_RES;
#else
        PDC1 = 0;
        PDC2 = 0;
        PDC3 = 0;
#endif
    } else if (data[1] == 1) {
        // run
        if (CONTROL_MODE != CONTROL_MODE_FOC_CUR_CONTROL) {
            STAT.b.RUN = 0;
            STAT.b.MOD = CTRL_MOD_CURRENT;
            FOC_Init_CurrentControl();
            CONTROL_MODE = CONTROL_MODE_FOC_CUR_CONTROL;
        }
    }
}

void CMD_FOC_Gain(uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        // set params
        Pgain_d1000 = Pgain_q1000 = (int) ((short) (data[2] | (data[3] << 8)));
        Igain_d1000 = Igain_q1000 = (int) ((short) (data[4] | (data[5] << 8)));
        Push_EEPROM_Ring(0, EE_TYPE_FOC_PGAIN, Pgain_q1000);
        Push_EEPROM_Ring(0, EE_TYPE_FOC_IGAIN, Igain_q1000);
    } else if (data[1] == 1) {
        // request params
        unsigned char canTxData[8] = {0,};
        canTxData[0] = CMD_FOC_GAIN;
        canTxData[1] = Pgain_q1000 & 0xFF;
        canTxData[2] = (Pgain_q1000 >> 8) & 0xFF;
        canTxData[3] = Igain_q1000 & 0xFF;
        canTxData[4] = (Igain_q1000 >> 8) & 0xFF;
        Push_CAN_TX_Ring(CANID_DATA, 5, canTxData);
    }
}

void CMD_FOC_OpenLoop_Control(uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        // stop
        CONTROL_MODE = CONTROL_MODE_NO_ACT;
#ifdef NEW_AMP2
        PDC1 = PWM_RES;
        PDC2 = PWM_RES;
        PDC3 = PWM_RES;
#else
        PDC1 = 0;
        PDC2 = 0;
        PDC3 = 0;
#endif
    } else if (data[1] == 1) {
        // run
        ref_voltage = (int) ((short) (data[2] | (data[3] << 8)))*10;
        if (CONTROL_MODE != CONTROL_MODE_FOC_OPENLOOP_CONTROL) {
            STAT.b.RUN = 0;
            FOC_Init_OpenLoopControl();
            CONTROL_MODE = CONTROL_MODE_FOC_OPENLOOP_CONTROL;
        }
    }
}


void CMD_POS_Gain(uint8_t data[], uint8_t dlc) {
    // Main Gain 1
    if (data[1] == 0) {
        // set params
        Pgain_pos = (int) ((unsigned short) (data[2] | (data[3] << 8)));
        Igain_pos = (int) ((unsigned short) (data[4] | (data[5] << 8)));
        Dgain_pos = (int) ((unsigned short) (data[6] | (data[7] << 8)));
        Push_EEPROM_Ring(0, EE_TYPE_POS_PGAIN, Pgain_pos);
        Push_EEPROM_Ring(0, EE_TYPE_POS_IGAIN, Igain_pos);
        Push_EEPROM_Ring(0, EE_TYPE_POS_DGAIN, Dgain_pos);

    } else if (data[1] == 1) {
        // request params
        unsigned char canTxData[8] = {0,};
        canTxData[0] = CMD_POS_GAIN;
        canTxData[1] = Pgain_pos & 0xFF;
        canTxData[2] = (Pgain_pos >> 8) & 0xFF;
        canTxData[3] = Igain_pos & 0xFF;
        canTxData[4] = (Igain_pos >> 8) & 0xFF;
        canTxData[5] = Dgain_pos & 0xFF;
        canTxData[6] = (Dgain_pos >> 8) & 0xFF;
        canTxData[7] = 1;
        Push_CAN_TX_Ring(CANID_DATA, 8, canTxData);
    }// Additional Gain 2
    else if (data[1] == 2) {
        // set params

    } else if (data[1] == 3) {
        // request params
        unsigned char canTxData[8] = {0,};
        canTxData[0] = CMD_POS_GAIN;
        canTxData[1] = 0;
        canTxData[2] = 0;
        canTxData[3] = 0;
        canTxData[4] = 0;
        canTxData[5] = 0;
        canTxData[6] = 0;
        canTxData[7] = 2;
        Push_CAN_TX_Ring(CANID_DATA, 8, canTxData);
    }// Additional Gain 3
    else if (data[1] == 4) {
        // set params
        
    } else if (data[1] == 5) {
        // request params
        unsigned char canTxData[8] = {0,};
        canTxData[0] = CMD_POS_GAIN;
        canTxData[1] = 0;
        canTxData[2] = 0;
        canTxData[3] = 0;
        canTxData[4] = 0;
        canTxData[5] = 0;
        canTxData[6] = 0;
        canTxData[7] = 3;
        Push_CAN_TX_Ring(CANID_DATA, 8, canTxData);
    }

}

void CMD_CurrentNulling(uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        // set params
        CurrentNulling();
        Push_EEPROM_Ring(0, EE_TYPE_CURRENT_NULL_RZ, RZ);
        Push_EEPROM_Ring(0, EE_TYPE_CURRENT_NULL_SZ, SZ);
        Push_EEPROM_Ring(0, EE_TYPE_CURRENT_NULL_TZ, TZ);

    } else if (data[1] == 1) {
        // request params
        unsigned char canTxData[8] = {0,};
        canTxData[0] = CMD_CURRENT_NULLING;
        canTxData[1] = RZ & 0xFF;
        canTxData[2] = (RZ >> 8) & 0xFF;
        canTxData[3] = SZ & 0xFF;
        canTxData[4] = (SZ >> 8) & 0xFF;
        canTxData[5] = TZ & 0xFF;
        canTxData[6] = (TZ >> 8) & 0xFF;
        Push_CAN_TX_Ring(CANID_DATA, 7, canTxData);
    } else if (data[1] == 5) {
        // set specific params
        RZ = (short) (data[2] | (data[3] << 8));
        SZ = (short) (data[4] | (data[5] << 8));
        TZ = (short) (data[6] | (data[7] << 8));
    } else if (data[1] == 10) {
//        if (data[2] == 0) {
//            if (data[3] == 0) {
//                RZ += 1;
//            } else {
//                RZ -= 1;
//            }
//        } else if (data[2] == 1) {
//            if (data[3] == 0) {
//                SZ += 1;
//            } else {
//                SZ -= 1;
//            }
//        } else if (data[2] == 2) {
//            if (data[3] == 0) {
//                TZ += 1;
//            } else {
//                TZ -= 1;
//            }
//        }
    }
}


void CMD_SineWaveTest(uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        // stop test -- goto
        goto_param.move_flag = 2;
        
    } else if (data[1] == 1) {
        // start test -- goto
        if (STAT.b.FET == 0 || STAT.b.RUN == 0){
            return;
        }
         if(speed_test_start != 0){
            return;
        }
        if(goto_param.move_flag != 0){
            return;
        }
        if(GO_POS_CNT < 3){
            return;
        }
        
        float target_pos = (float)((int) (data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24)));
        float target_vel = (float)((int) ((unsigned short) (data[6] | (data[7] << 8)))); // pulse/tick
        
        if(target_pos > POS_UPPER_LIM){
            target_pos = POS_UPPER_LIM;
        }
        if(target_pos < POS_LOWER_LIM){
            target_pos = POS_LOWER_LIM;
        }
        
        if(fabs(goto_param.cur_pos - (currentMotorPosition)) > 1000){
            set_goto_start_pos(currentMotorPosition);
        }else{
            set_goto_start_pos(goto_param.cur_pos);
        }
        set_goto_target(target_pos, target_vel);
        
        CONTROL_MODE = CONTROL_MODE_FOC_POS_CONTROL;
        
    } else if (data[1] == 2) {
        // stop test -- speed
        speed_test_start = 2;
        
    } else if (data[1] == 3) {
        // start test -- speed
        if (STAT.b.FET == 0 || STAT.b.RUN == 0) {
            return;
        }
        if(speed_test_start != 0){
            return;
        }
        if(goto_param.move_flag != 0){
            return;
        }
        if(GO_POS_CNT < 3){
            return;
        }
        
        int target_rpm = (int) (data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24));
        speed_test_acc = (int) ((unsigned short) (data[6] | (data[7] << 8)));

        speed_test_target = target_rpm * ENC_RES / 500 / 60;
        speed_test_acc = fabs(speed_test_target) / 200;
        if (speed_test_acc < 1)
            speed_test_acc = 1;
        if(speed_test_start == 0){
            speed_test_cur_speed = 0;
        }
        speed_test_start = 1;
        CONTROL_MODE = CONTROL_MODE_FOC_POS_CONTROL;
        
    }
}

void CMD_MuDiskOffset(uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        // set mu disk offset
    } else if (data[1] == 1) {
        // request 
    }
}

void CMD_HomeZeroOffset(uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        // set zero offset
        if (data[6] == 1) {
            // set cur pos zero
            HOME_ZERO_OFFSET = NewPos;
        } else {
            HOME_ZERO_OFFSET = (int) (data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24));
        }
        Push_EEPROM_Ring(0, EE_TYPE_HOME_ZERO_OFFSET, HOME_ZERO_OFFSET);
    } else if (data[1] == 1) {
        // request zero offset
        unsigned char canTxData[8] = {0,};
        canTxData[0] = CMD_HOME_ZERO_OFFSET;
        canTxData[1] = HOME_ZERO_OFFSET & 0xFF;
        canTxData[2] = (HOME_ZERO_OFFSET >> 8) & 0xFF;
        canTxData[3] = (HOME_ZERO_OFFSET >> 16) & 0xFF;
        canTxData[4] = (HOME_ZERO_OFFSET >> 24) & 0xFF;
        Push_CAN_TX_Ring(CANID_DATA, 5, canTxData);
    }
}

void CMD_EncResDir(uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        // set enc resolution & pole number & direction
        Encoder_res = (int) (data[2] | (data[3] << 8));
        AUTO_SCALE_FLAG = data[4];
        //Pole_number = data[4];


//        if (MDR == data[5]) {
//            MDR = data[5];
//        } else {
//            MDR = data[5];
//            HOME_ZERO_OFFSET = -HOME_ZERO_OFFSET;
//            Push_EEPROM_Ring(0, EE_TYPE_HOME_ZERO_OFFSET, HOME_ZERO_OFFSET);
//        }
        if (HomeDir == data[5]) {
            HomeDir = data[5];
        } else {
            HomeDir = data[5];
            HOME_ZERO_OFFSET = -HOME_ZERO_OFFSET;
            Push_EEPROM_Ring(0, EE_TYPE_HOME_ZERO_OFFSET, HOME_ZERO_OFFSET);
        }

        Push_EEPROM_Ring(0, EE_TYPE_ENC_RES, Encoder_res);
        //Push_EEPROM_Ring(0, EE_TYPE_POLE_NUM, Pole_number);
        Push_EEPROM_Ring(0, EE_TYPE_AUTO_SCALE, AUTO_SCALE_FLAG);
//        Push_EEPROM_Ring(0, EE_TYPE_MOTOR_DIR, MDR);
        Push_EEPROM_Ring(0, EE_TYPE_MOTOR_DIR, HomeDir);
    } else if (data[1] == 1) {
        // request enc resolution & pole number & direction
        unsigned char canTxData[8] = {0,};
        canTxData[0] = CMD_ENC_RES_DIR;
        canTxData[1] = Encoder_res & 0xFF;
        canTxData[2] = (Encoder_res >> 8) & 0xFF;
        //canTxData[3] = Pole_number;
        canTxData[3] = AUTO_SCALE_FLAG;
//        canTxData[4] = MDR;
        canTxData[4] = HomeDir;
        Push_CAN_TX_Ring(CANID_DATA, 5, canTxData);
    }
}

void CMD_MU150(uint8_t data[], uint8_t dlc) {
    if (data[1] == 0) {
        // Calibration Mode
        MU_CALIBRATION_MODE = data[2];
        if (MU_CALIBRATION_MODE == 1) {
//            ResumeTimer2();
            STAT.b.CALIB = 1;
        } else {
//            SuspendTimer2();
            STAT.b.CALIB = 0;
        }
    } else if (data[1] == 1) {
        // st mode change
        if (data[2] == 0) {
            // abs data mode
            Push_MU_Ring(0x12, 0);
            Push_MU_Ring(0x11, (5 << 5) | 5);
            MU_ST_MODE = data[2];
        } else if (data[2] == 2) {
            // raw data mode
            Push_MU_Ring(0x12, 2 << 4);
            Push_MU_Ring(0x11, (4 << 5) | 14);
            MU_ST_MODE = data[2];
        } else if (data[2] == 10) {
            // multi-turn mode
            Push_MU_Ring(0x12, 0);
            Push_MU_Ring(0x11, (5 << 5) | 21);
            MU_ST_MODE = data[2];
        }
    } else if (data[1] == 2) {
        // Request EEPROM
        mu_send_eeprom_cnt = 0;
        mu_send_eeprom_flag = 0;
        mu_req_eeprom_cnt = 0;
        mu_req_eeprom_flag = 1;
    } else if (data[1] == 3) {
        // Write EEPROM
        int addr = data[2];
        int value = data[3];
        Push_MU_Ring(addr, value);
    } else if (data[1] == 4) {
        // Disable Servo
        if(STAT.b.RUN == 1){
            return;
        }
        if(CONTROL_MODE == CONTROL_MODE_FOC_POS_CONTROL){
            return;
        }
        if(CONTROL_MODE == CONTROL_MODE_FOC_HOME_CONTROL){
            return;
        }
        CONTROL_MODE = CONTROL_MODE_NO_ACT;
        GateDriver_OFF();
        
//        CONTROL_MODE = CONTROL_MODE_NO_ACT;
//        Ir2131_OFF();
        
//        // Pre Pulse Shot
//        PRE_PULSE_FLAG = 1;
//        
//        if (MU_CALIBRATION_MODE == 0) {
//            ResumeTimer2();
//            STAT.b.CALIB = 1;
//        }
    } else if (data[1] == 5) {
        // Disable Servo
        if(STAT.b.RUN == 1){
            return;
        }
        if(CONTROL_MODE == CONTROL_MODE_FOC_POS_CONTROL){
            return;
        }
        if(CONTROL_MODE == CONTROL_MODE_FOC_HOME_CONTROL){
            return;
        }
        CONTROL_MODE = CONTROL_MODE_NO_ACT;
        GateDriver_OFF();
        
        // Shot Preset and New Home Set Process
//        PRESET_AND_NEW_HOME_POS_FLAG = 1;
//        PRE_PULSE_FLAG = 1;
        SAVE_MU_EEPROM_FLAG = 1;
        if (MU_CALIBRATION_MODE == 0) {
//            ResumeTimer2();
            STAT.b.CALIB = 1;
        }
    } else if (data[1] == 7) {
        if (data[2] == 1) {
            STAT.b.SPI_EXT = 1;
            SPI1CONbits.ON = 0;
            TRISBbits.TRISB5 = 1;
        } else {
            STAT.b.SPI_EXT = 0;
            SPI1CONbits.ON = 1;
            TRISBbits.TRISB5 = 0;
        }
    } else if (data[1] == 10) {
        // do setting
        mu_setting_stat = 0;
        mu_setting_flag = 1;
    } else if (data[1] == 11) {
        // do checking
        mu_check_stat = 0;
        mu_check_flag = 1;
    } else if (data[1] == 77) {
        // Firmware Reset
        int a = 37 / 0;
    }
}


// Functional Safety --------------------------------------------

static unsigned char SF_SEQ = 0;
static short SF_VEL1 = 0;
static short SF_VEL2 = 0;
static char SF_CUR1 = 0;
static char SF_CUR2 = 0;

void SafetyDataSend_Normal(){
    unsigned char canTxData[8] = {0,};
        
    SF_VEL1 = (short)(VEL);
    SF_VEL2 = (short)(VEL2);
    
    canTxData[0] = SF_SEQ;
    canTxData[1] = SF_VEL1 & 0xFF;
    canTxData[2] = (SF_VEL1 >> 8) & 0xFF;
    canTxData[3] = SF_VEL2 & 0xFF;
    canTxData[4] = (SF_VEL2 >> 8) & 0xFF;
    CAN_WRITE_TMR3(CANID_SF_NORMAL, 5, canTxData);
}

void SafetyDataSend_Invert(){
    unsigned char canTxData[8] = {0,};
    
    canTxData[0] = ~(SF_SEQ);
    canTxData[1] = ~(SF_VEL1 & 0xFF);
    canTxData[2] = ~((SF_VEL1 >> 8) & 0xFF);
    canTxData[3] = ~(SF_VEL2 & 0xFF);
    canTxData[4] = ~((SF_VEL2 >> 8) & 0xFF);
    CAN_WRITE_TMR3(CANID_SF_INVERT, 5, canTxData);
            
    SF_SEQ = (++SF_SEQ)%256;
}