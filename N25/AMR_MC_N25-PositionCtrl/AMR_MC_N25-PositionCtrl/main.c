#include <proc/p32mk1024mcf064.h>
#include <../xc.h>

#include <sys/attribs.h>
#include <math.h>

#include "cpu_config.h"
#include "board_definition.h"
#include "init.h"
#include "timer.h"
#include "port.h"
#include "can.h"
#include "adc.h"
#include "qei.h"
#include "spi.h"
#include "status.h"
#include "control.h"
#include "execution.h"
#include "FOC.h"
#include "pwm.h"
#include "home.h"
#include "SV_table.h"


static unsigned long timerCnt = 0;

static long long init_start_pos = 0;
static int init_enc_error_cnt = 0;
static int mu_pvl_reset_flag = 0;

static void MU_CALIB_MODE_FUNCTION(unsigned long cnt);
static void MU150_SETTING_HALT(int errno);
static void MU150_CHECK_HALT(int errno);

static void SPEED_CONTROL_LOOP();
static void GOTO_CONTROL_LOOP();
static void CheckError_TurnOff();
static void InitControlHandler();


extern short send_current_q;

#define INIT_SHAKE_PULSE    1600
#define INIT_SHAKE_CNT      200


int main(){        
    // cpu acceleration setting -------------------
    asm volatile("di");
    CHECONbits.PFMWS = 0b011;
    CHECONbits.PREFEN = 0b01;
    register unsigned long tmp;
    asm("mfc0 %0,$16,0" :  "=r"(tmp));
    tmp = (tmp & ~7) | 3;
    asm("mtc0 %0,$16,0" :: "r" (tmp));
    // --------------------------------------------
    
    
    // apply initial delay for stable operation ---
    int initial_delay;
    for(initial_delay=0; initial_delay<100000; initial_delay++)
        ;
    for(initial_delay=0; initial_delay<100000; initial_delay++)
        ;
    volatile int initial_delay2 = 12000000;
    while(initial_delay2--)
        ;
    // --------------------------------------------
    
    // multi-vector enable
    INTCONbits.MVEC = 1;
    
    
    ANSELA = 0;
    ANSELB = 0;
    ANSELC = 0;
    ANSELD = 0;
    ANSELE = 0;
    ANSELF = 0;
    ANSELG = 0;
        
                
    
    // Initialize -----
//    InitPVL();    
    InitBoardIdentification();
    InitPeripheral();
    
    
    //MOTOR_HIP_OFF
    GateDriver_OFF();
    asm volatile("ei");
    
    
    
    
    EEPROM_DESELECT
    MU150_DESELECT
    MU150_DESELECT2
//    ACC_DESELECT
    
    
    int i, j ;
    for(i=0; i<1000; ++i)
        for(j=0; j<800; ++j)
            ;   
    EncoderReset_from_MU150();
    
    int temp_mu_stat = MU150_ReadRegister(0x77);
    if(((temp_mu_stat>>8)>>3) & 0x01){
        // NON_CTR
        STAT.b.NON_CTR = 1;
    }
    
    
    unsigned char canTxData[8] = {0,};
    canTxData[0] = 0;
    Push_CAN_TX_Ring(900+JMC, 1, canTxData); 
    
    
    goto_param.move_flag = 0;
    goto_param.fixed_a = 1.0;   // 1pulse/tick^2
    home_param.move_flag = 0;
    
    
    // pumping------
//    GateDriver_ON();
//    for(k=0;k<100000;k++)
//        ;
//    GateDriver_OFF();
            
    
//    TRISBbits.TRISB5 = 1;
//    SPI1CONbits.ON = 0;
     
    
    unsigned long while_cnt = 0;
    while(1){
        while_cnt++;
        
        // CAN Handler
        Handler_CAN_TX_Ring();
        Handler_CAN_TX_Ring_TMR3();
              
        
        // Error Handler
//        Handler_Error(timerCnt);
        
        // Status Handler
        Handler_Status(timerCnt);

              
        if(InitControlFlag == 1){
            InitControlHandler();
        }else{
            // EEPROM Handler
            if(while_cnt%100 == 0){
                Handler_EEPROM_Ring();
            }else{
                if(DQ_SAVE == 1){
                    DQ_SAVE = 0;
                    int tempMU150 = MU150_ReadSensor()/8;
     
                    InitPos_DQ_MU = tempMU150;
                    Push_EEPROM_Ring(0, EE_TYPE_INIT_DQ_MU_POS, InitPos_DQ_MU);
                    
                }else if(MU_CALIBRATION_MODE == 1){
                    MU_CALIB_MODE_FUNCTION(timerCnt);
                }
            }
        }
                
        Handler_ADC_Trigger(timerCnt);
                       
        if(timerCnt % 200 == 0){
            LATAbits.LATA12 ^= 1;
        }        
    }
}



unsigned int control_sync_cnt = 0;
unsigned int t3_cnt = 0;
extern int send_encoder_counter;
void __ISR(_TIMER_3_VECTOR, IPL6AUTO) Handler_TMR_3(){
    static int time_shift_for_encoder_send = 0;
    
//    LATAbits.LATA4 = 1;
    while(!ADCDSTAT1bits.ARDY0)
        ;
    while(!ADCDSTAT1bits.ARDY1)
        ;
    while(!ADCDSTAT1bits.ARDY2)
        ;    
    
    
    ++t3_cnt;
    UpdateEncoder();

    switch(CONTROL_MODE){

    case CONTROL_MODE_FOC_POS_CONTROL:
        if(t3_cnt%40 == 0){
            if(STAT.b.RUN == 1)
            {
                GO_POS_CNT++;
            }
            
            if(INPUT_DATA_ABS == 0)
            {
                // mobile speed command
                Handler_Control_Diff();
            }
            else
            {
                // hubo-i abs pos command
                if(GO_POS_CNT > 2)
                {
                    SPEED_CONTROL_LOOP();
                    GOTO_CONTROL_LOOP();
                    Handler_Control_Abs();
                }
            }
        }
        FOC_CurrentControl_Double();
        
        break;
        
    case CONTROL_MODE_FOC_HOME_CONTROL:
        if(t3_cnt%40 == 0){
            move_home_target();
            NewInputRefAbs = home_param.cur_pos;
            Handler_Control_Abs();
        }
        FOC_CurrentControl_Double();
        break;

    case CONTROL_MODE_FOC_CUR_CONTROL:
        ref_current_q = NewInputCurrent;
        GO_CUR_CNT++;
//        if(GO_CUR_CNT > 2000){
//            STAT.b.CAN_ERR = 1;
//        }
        FOC_CurrentControl_Double();
        break;

    case CONTROL_MODE_FOC_OPENLOOP_CONTROL:
        FOC_MeasureCurrent();
        FOC_OpenLoopControl();            
        break;
        
    case CONTROL_MODE_STO:
        solenoid_stat = SOLENOID_STAT_TURN_OFF;
        PDC1 = PWM_STOP;
        PDC2 = PWM_STOP;
        PDC3 = PWM_STOP;
        GateDriver_OFF();
        CONTROL_MODE = CONTROL_MODE_NO_ACT;
        break;
        
    case CONTROL_MODE_SS1_POS:
//        if(t3_cnt%40 == control_sync_cnt){
        if(t3_cnt%40 == 0){
            if(SS1_CNT < SS1_TARGET_CNT){
                SS1_CNT++;
                if(INPUT_DATA_ABS == 0){
                    // diff mode
                    NewInputRefDiff = MUL * SS1_VEL * (SS1_TARGET_CNT-SS1_CNT) / SS1_TARGET_CNT;
                    Handler_Control_Diff();
                }else{
                    // abs mode
                    NewInputRefAbs = REF + SS1_VEL * (SS1_TARGET_CNT-SS1_CNT) / SS1_TARGET_CNT;
                    Handler_Control_Abs();
                }
            }else{
                solenoid_stat = SOLENOID_STAT_TURN_OFF;
                PDC1 = PWM_STOP;
                PDC2 = PWM_STOP;
                PDC3 = PWM_STOP;
                GateDriver_OFF();
                CONTROL_MODE = CONTROL_MODE_NO_ACT;
                break;  // important
            }
        }
        FOC_CurrentControl_Double();
        break;
        
    case CONTROL_MODE_SS2_POS:
//        if(t3_cnt%40 == control_sync_cnt){
        if(t3_cnt%40 == 0){
            if(SS1_CNT < SS1_TARGET_CNT){
                SS1_CNT++;
                if(INPUT_DATA_ABS == 0){
                    // diff mode
                    NewInputRefDiff = MUL * SS1_VEL * (SS1_TARGET_CNT-SS1_CNT) / SS1_TARGET_CNT;
                    Handler_Control_Diff();
                }else{
                    // abs mode
                    NewInputRefAbs = REF + SS1_VEL * (SS1_TARGET_CNT-SS1_CNT) / SS1_TARGET_CNT;
                    Handler_Control_Abs();
                }
            }else{
                if(INPUT_DATA_ABS == 0){
                    Handler_Control_Diff();
                }else{
                    SPEED_CONTROL_LOOP();
                    GOTO_CONTROL_LOOP();
                    Handler_Control_Abs();
                }
            }
        }
        FOC_CurrentControl_Double();
        break;
        
        
    case CONTROL_MODE_NO_ACT:
    default:
        FOC_MeasureCurrent();
        break;
    }

     
    MV_AVG_Q();

    if(t3_cnt%20 == 0){ // 1kHz
        timerCnt++;
        Handler_Solenoid();
        if(timerCnt%1000 == 0){
            LATEbits.LATE13 ^= 1;
        }
        ReadEncoder();
    }
    
    
    if(ENC_SHOT_CNT > 0){
        if(t3_cnt%2 == 0){
            --ENC_SHOT_CNT;
        }
    }else if(ENC_SHOT_CNT == 0){
        --ENC_SHOT_CNT;
        CMD_SendEncoder();
    }else{
        // watchdog for sending encoder 
        if(t3_cnt%40 == 0){            
            REF_MSG_CNT++;
            if(REF_MSG_CNT > 2){
                encoder_send_cnt++;
                CMD_SendEncoder();
            }
        }
    }
    
    if(send_encoder_counter >= 5){
        time_shift_for_encoder_send++;
        if(time_shift_for_encoder_send >= 2){
            time_shift_for_encoder_send = 0;
            send_encoder_counter = 0;
            CMD_SendEncoder_Huboi();
        }
    }
    
    
    // Functional Safety Data Send----------
    if(SAFETY_DATA_ONOFF == 1){
        if(t3_cnt%400 == 15){
            // normal data (encoder/current)
            SafetyDataSend_Normal();
        }else if(t3_cnt%400 == 35){
            // inverted data (encoder/current)
            SafetyDataSend_Invert();
        }
    }
    //---------------------------------------
    
    
    
    CheckError_TurnOff();
    
    IFS0bits.T3IF=0;
}


// 1kHz Timer
int tmr2_cnt = 0;
int MU150_EEPROM_DATA[200];
int MU150_EEPROM_STAT[200];


void MU_CALIB_MODE_FUNCTION(unsigned long cnt){
    static unsigned long prev_cnt = 0;
    static unsigned long mu_cnt = 0;
    
    if(cnt - prev_cnt > 0){
        prev_cnt = cnt;
        mu_cnt++;
    
        
        if(mu_cnt%10 == 0 && MU_ST_MODE == 0){
            // Abs Data
            int mu_data = MU150_ReadSensor()/8;

            unsigned char canTxData[8] = {0,};
            canTxData[0] = 0;
            canTxData[1] = mu_data&0xFF;
            canTxData[2] = (mu_data>>8)&0xFF;
            canTxData[3] = (mu_data>>16)&0xFF;
            canTxData[4] = (mu_data>>24)&0xFF;
            canTxData[5] = 2;
            canTxData[6] = 0 & 0xFF;
            canTxData[7] = (0>>8) & 0xFF;
            Push_CAN_TX_Ring(600+JMC, 8, canTxData); 
        }
    
        if(mu_req_eeprom_flag){
            if(mu_req_eeprom_cnt <= 0x65){//0x7F){//0x82){
                int temp_data = MU150_ReadRegister(mu_req_eeprom_cnt);
                MU150_EEPROM_STAT[mu_req_eeprom_cnt] = temp_data&0xFF;
                MU150_EEPROM_DATA[mu_req_eeprom_cnt] = (temp_data>>8)&0xFF;
                mu_req_eeprom_cnt++;
            }else{
                mu_req_eeprom_cnt = 0;
                mu_req_eeprom_flag = 0;
                mu_send_eeprom_cnt = 0;
                mu_send_eeprom_flag = 1;
            }
        }else if(mu_send_eeprom_flag){
            if(mu_send_eeprom_cnt <= 0x65){//0x7F){//0x82){
                unsigned char canTxData[8] = {0,};
                canTxData[0] = 1;
                canTxData[1] = 1;
                canTxData[2] = mu_send_eeprom_cnt;
                canTxData[3] = MU150_EEPROM_STAT[mu_send_eeprom_cnt];
                canTxData[4] = MU150_EEPROM_DATA[mu_send_eeprom_cnt];
                Push_CAN_TX_Ring(600+JMC, 5, canTxData);
                mu_send_eeprom_cnt++;
            }else{
                unsigned char canTxData[8] = {0,};
                canTxData[0] = 1;
                canTxData[1] = 2;
                Push_CAN_TX_Ring(600+JMC, 2, canTxData);    // finish signal
                mu_send_eeprom_cnt = 0;
                mu_send_eeprom_flag = 0;
            }
        }else if(mu_setting_flag == 1){
            if(mu_setting_stat == 0){
                bank_0_index = 0;
                mu_setting_stat = 1;
            }else if(mu_setting_stat == 1){
                MU_EEPROM_SETTING_BANK_0(bank_0_index);
                bank_0_index++;
                if(bank_0_index > 5){
                    MU150_WriteRegister(0x75, 0x01);    // write eeprom
                    bank_0_index = 0;
                    mu_setting_stat = 2;
                }
            }else if(mu_setting_stat == 2){
                MU_EEPROM_SETTING_BANK_0_2(bank_0_index);
                bank_0_index++;
                if(bank_0_index > 5){
                    MU150_WriteRegister2(0x75, 0x01);    // write eeprom
                    MU150_SETTING_HALT(100);

                    mu_setting_stat = 0;
                    mu_setting_flag = 0;
                }
            }

        }else if(mu_check_flag == 1){
            if(mu_check_stat == 0){
                if(MU_EEPROM_CHECK_BANK_0() == 1){
                    if(MU_EEPROM_CHECK_BANK_0_2() == 1){
                        MU150_CHECK_HALT(100);
                    }else{
                        MU150_CHECK_HALT(2);
                    }
                }else{
                    MU150_CHECK_HALT(1);
                }
                mu_check_stat = 0;
                mu_check_flag = 0;
            }        
        }
        else if(mu_cnt%10 == 3){
            // read status
            int temp_data = MU150_ReadRegister(0x77);
            unsigned char canTxData[8] = {0,};
            canTxData[0] = 2;
            canTxData[1] = 1;
            canTxData[2] = 0x77;
            canTxData[3] = temp_data&0xFF;
            canTxData[4] = (temp_data>>8)&0xFF;
            Push_CAN_TX_Ring(600+JMC, 5, canTxData);
        }else if(SAVE_MU_EEPROM_FLAG == 1){
            SAVE_MU_EEPROM_FLAG = 0;
            MU150_WriteRegister(0x75, 0x01);    // write eeprom
        }
        else{
            Handler_MU_Ring();
        }
    }
}


void __ISR(_TIMER_2_VECTOR, IPL4AUTO) Handler_TMR_2(){
    tmr2_cnt++;
    
    if(mu_pvl_reset_flag == 1){
        IFS0bits.T2IF=0;
        return;
    }
    if(MU_ST_MODE == 0){
        // Abs Data
        int mu_data = MU150_ReadSensor();
        
        unsigned char canTxData[8] = {0,};
        canTxData[0] = 0;
        canTxData[1] = mu_data&0xFF;
        canTxData[2] = (mu_data>>8)&0xFF;
        canTxData[3] = (mu_data>>16)&0xFF;
        canTxData[4] = (mu_data>>24)&0xFF;
        canTxData[5] = 0;
        Push_CAN_TX_Ring(600+JMC, 6, canTxData);
    }else if(MU_ST_MODE == 2){
        // Raw Data
        int mu_data = MU150_ReadSensorRaw();
        
        unsigned char canTxData[8] = {0,};
        canTxData[0] = 0;
        canTxData[1] = mu_data&0xFF;
        canTxData[2] = (mu_data>>8)&0xFF;
        canTxData[3] = (mu_data>>16)&0xFF;
        canTxData[4] = (mu_data>>24)&0xFF;
        canTxData[5] = 1;
        Push_CAN_TX_Ring(600+JMC, 6, canTxData);
    }else if(MU_ST_MODE == 10){
        // Abs Multi-turn Data
        int mu_data = MU150_ReadSensor_MultiTurn();
        
        unsigned char canTxData[8] = {0,};
        canTxData[0] = 0;
        canTxData[1] = mu_data&0xFF;
        canTxData[2] = (mu_data>>8)&0xFF;
        canTxData[3] = (mu_data>>16)&0xFF;
        canTxData[4] = (mu_data>>24)&0xFF;
        canTxData[5] = 2;
        canTxData[6] = MULTI_TURN_DATA & 0xFF;
        canTxData[7] = (MULTI_TURN_DATA>>8) & 0xFF;
        Push_CAN_TX_Ring(600+JMC, 8, canTxData);
    }
    
    
    if(mu_req_eeprom_flag){
        if(mu_req_eeprom_cnt <= 0x65){//0x7F){//0x82){
            int temp_data = MU150_ReadRegister(mu_req_eeprom_cnt);
            MU150_EEPROM_STAT[mu_req_eeprom_cnt] = temp_data&0xFF;
            MU150_EEPROM_DATA[mu_req_eeprom_cnt] = (temp_data>>8)&0xFF;
            mu_req_eeprom_cnt++;
        }else{
            mu_req_eeprom_cnt = 0;
            mu_req_eeprom_flag = 0;
            mu_send_eeprom_cnt = 0;
            mu_send_eeprom_flag = 1;
        }
    }else if(mu_send_eeprom_flag){
        if(mu_send_eeprom_cnt <= 0x65){//0x7F){//0x82){
            unsigned char canTxData[8] = {0,};
            canTxData[0] = 1;
            canTxData[1] = 1;
            canTxData[2] = mu_send_eeprom_cnt;
            canTxData[3] = MU150_EEPROM_STAT[mu_send_eeprom_cnt];
            canTxData[4] = MU150_EEPROM_DATA[mu_send_eeprom_cnt];
            Push_CAN_TX_Ring(600+JMC, 5, canTxData);
            mu_send_eeprom_cnt++;
        }else{
            unsigned char canTxData[8] = {0,};
            canTxData[0] = 1;
            canTxData[1] = 2;
            Push_CAN_TX_Ring(600+JMC, 2, canTxData);    // finish signal
            mu_send_eeprom_cnt = 0;
            mu_send_eeprom_flag = 0;
        }
    }
//    else if(mu_setting_flag == 1){
//        if(mu_setting_stat == 0){
//            // change banksel 0
//            MU150_WriteRegister(0x40, 0);
//            mu_setting_stat = 1;
//        }else if(mu_setting_stat == 1){
//            // check banksel 0
//            unsigned int temp = (MU150_ReadRegister(0x40)>>8)&0xFF;
//            if(temp == 0){
//                bank_0_index = 0;
//                mu_setting_stat = 2;
//            }else{
//                MU150_SETTING_HALT(1);
//            }
//        }else if(mu_setting_stat == 2){
//            // write bank 0
//            MU_EEPROM_SETTING_BANK_0(bank_0_index);
//            bank_0_index++;            
//            if(bank_0_index > 5){
//                mu_setting_stat = 3;
//            }
//        }else if(mu_setting_stat == 3){
//            // change banksel 0
//            MU150_WriteRegister(0x40, 1);
//            mu_setting_stat = 4;
//        }else if(mu_setting_stat == 4){
//            // check banksel 1
//            unsigned int temp = (MU150_ReadRegister(0x40)>>8)&0xFF;
//            if(temp == 1){
//                bank_1_index = 0;
//                bank_1_cnt = 0;
//                mu_setting_stat = 5;
//            }else{
//                MU150_SETTING_HALT(4);
//            }
//        }else if(mu_setting_stat == 5){
//            // write bank 1
//            if(bank_1_cnt % 10 == 0){
//                MU_EEPROM_SETTING_BANK_1(bank_1_index);
//                bank_1_index++;
//            }
//            bank_1_cnt++;
//            
//            if(bank_1_index > 12){
//                MU150_SETTING_HALT(100);
//                mu_setting_stat = 0;
//                mu_setting_flag = 0;
//            }
//        }
//    }else if(mu_check_flag == 1){
//        if(mu_check_stat == 0){
//            // change banksel 0
//            MU150_WriteRegister(0x40, 0);
//            mu_check_stat = 1;
//        }else if(mu_check_stat == 1){
//            // check banksel 0
//            unsigned int temp = (MU150_ReadRegister(0x40)>>8)&0xFF;
//            if(temp == 0){
//                mu_check_stat = 2;
//            }else{
//                MU150_CHECK_HALT(1);
//            }
//        }else if(mu_check_stat == 2){
//            // check bank 0
//            if(MU_EEPROM_CHECK_BANK_0() == 1){
//                mu_check_stat = 3;
//            }else{
//                MU150_CHECK_HALT(2);
//            }
//        }else if(mu_check_stat == 3){
//            // change banksel 0
//            MU150_WriteRegister(0x40, 1);
//            mu_check_stat = 4;
//        }else if(mu_check_stat == 4){
//            // check banksel 1
//            unsigned int temp = (MU150_ReadRegister(0x40)>>8)&0xFF;
//            if(temp == 1){
//                mu_check_stat = 5;
//            }else{
//                MU150_CHECK_HALT(4);
//            }
//        }else if(mu_check_stat == 5){
//            // check bank 1
//            if(MU_EEPROM_CHECK_BANK_1() == 1){
//                MU150_CHECK_HALT(100);
//            }else{
//                MU150_CHECK_HALT(5);
//            }
//            mu_check_stat = 0;
//            mu_check_flag = 0;
//        }
//    }
    else if(mu_setting_flag == 1){
        if(mu_setting_stat == 0){
            bank_0_index = 0;
            mu_setting_stat = 1;
        }else if(mu_setting_stat == 1){
            MU_EEPROM_SETTING_BANK_0(bank_0_index);
            bank_0_index++;            
            if(bank_0_index > 5){
                MU150_WriteRegister(0x75, 0x01);
                MU150_SETTING_HALT(100);
                
                mu_setting_stat = 0;
                mu_setting_flag = 0;
            }
        }
    }else if(mu_check_flag == 1){
        if(mu_check_stat == 0){
            if(MU_EEPROM_CHECK_BANK_0() == 1){
                MU150_CHECK_HALT(100);
            }else{
                MU150_CHECK_HALT(1);
            }
            mu_check_stat = 0;
            mu_check_flag = 0;
        }
    }
    
    else if(tmr2_cnt%10 == 0){
        // read status
        int temp_data = MU150_ReadRegister(0x77);
        unsigned char canTxData[8] = {0,};
        canTxData[0] = 2;
        canTxData[1] = 1;
        canTxData[2] = 0x77;
        canTxData[3] = temp_data&0xFF;
        canTxData[4] = (temp_data>>8)&0xFF;
        Push_CAN_TX_Ring(600+JMC, 5, canTxData);
    }
    else{
        Handler_MU_Ring();
    }

    
    if(PRE_PULSE_FLAG == 1){
        PRESET_ON
        PRE_PULSE_FLAG = 2;
    }else if(PRE_PULSE_FLAG >= 2){
        PRE_PULSE_FLAG++;
        if(PRE_PULSE_FLAG == 100){
            PRESET_OFF
//            PRE_PULSE_FLAG = 0;
        }else if(PRE_PULSE_FLAG == 200){
            mu_pvl_reset_flag = 1;
            PRE_PULSE_FLAG = 0;
            if(MU_CALIBRATION_MODE == 0){
                SuspendTimer2();
                STAT.b.CALIB = 0;
            }
        }
    }
    
    IFS0bits.T2IF=0;
}


void MU150_SETTING_HALT(int errno){
    unsigned char canTxData[8] = {0,};
    canTxData[0] = 10;
    canTxData[1] = errno;
    Push_CAN_TX_Ring(600+JMC, 2, canTxData);
    mu_setting_stat = 0;
    mu_setting_flag = 0;
}

void MU150_CHECK_HALT(int errno){
    unsigned char canTxData[8] = {0,};
    canTxData[0] = 11;
    canTxData[1] = errno;
    Push_CAN_TX_Ring(600+JMC, 2, canTxData);
    mu_check_stat = 0;
    mu_check_flag = 0;
}


void SPEED_CONTROL_LOOP(){    
    if(speed_test_start == 1){
        //speed_test_cur_speed += speed_test_acc;
        if(speed_test_cur_speed < speed_test_target){// >= 0){
            speed_test_cur_speed += speed_test_acc;
            if(speed_test_cur_speed > speed_test_target){
                speed_test_cur_speed = speed_test_target;
            }
        }else if(speed_test_cur_speed > speed_test_target){
            speed_test_cur_speed -= speed_test_acc;
            if(speed_test_cur_speed < speed_test_target){
                speed_test_cur_speed = speed_test_target;
            }
        }
        int prev_ref = REF;
        int itemp = prev_ref + speed_test_cur_speed;
        NewInputRefAbs = itemp;
        
    }else if(speed_test_start == 2){
        if(speed_test_cur_speed >= 0){
            speed_test_cur_speed -= speed_test_acc;
            if(speed_test_cur_speed <= 0){
                speed_test_cur_speed = 0;
                speed_test_start = 0;
            }
        }else{
            speed_test_cur_speed += speed_test_acc;
            if(speed_test_cur_speed >= 0){
                speed_test_cur_speed = 0;
                speed_test_start = 0;
            }
        }
        int prev_ref;
        prev_ref = REF;
        int itemp = prev_ref + speed_test_cur_speed;
        NewInputRefAbs = itemp;
    }
}

void GOTO_CONTROL_LOOP(){
    if(speed_test_start == 0){
        if(goto_param.move_flag != 0){
            move_goto_target();
            NewInputRefAbs = goto_param.cur_pos;
        }
    }else{
        goto_param.cur_pos = NewInputRefAbs;
    }
}

void CheckError_TurnOff(){
    if(((STAT.B & 0x00000F00) != 0) || (STAT.b.CUR_BIG == 1) || (STAT.b.CAN_ERR == 1)){
        if(ERROR_FLAG == 0){
            ERROR_FLAG = 1;
            solenoid_stat = SOLENOID_STAT_TURN_OFF;
            GateDriver_OFF();
            CONTROL_MODE = CONTROL_MODE_NO_ACT;
        }
    }
}

void InitControlHandler(){    
    
    switch(InitControlState){
    case 1:
        GateDriver_OFF();
        CONTROL_MODE = CONTROL_MODE_NO_ACT;

        // solenoid hard push
        sole_hard_push_cnt = SOLENOID_HARD_PUSH_COUNT;
        solenoid_stat = SOLENOID_STAT_HARD_PUSH;
        MT_ON = 1;
        InitControlState = 2;
        break;

    case 2: 
        CMD_FetEnable(1);
        CMD_RunClosedLoop(1, 0);
        InitControlState = 8;
//        InitControlState = 10;
        break;
        
    case 8:
        if(home_param.move_flag == 0){
            STAT.b.INIT = 1;
            InitControlState = 0;
            InitControlFlag = 0;
            
            CONTROL_MODE = CONTROL_MODE_FOC_POS_CONTROL;
        }
        break;
    
    // Init for brake (shaking motion)
    case 10:
        sole_shake_push_cnt = 500;
        solenoid_stat = SOLENOID_STAT_SHAKE_PUSH;

        init_start_pos = currentMotorPosition = currentMotorPosition2 = NewInputRefAbs = REF = PREV_REF = NewPos = NewPos2 = OldPos = OldPos2 = 0;
        set_home_start_pos(init_start_pos);
        set_home_target(init_start_pos+INIT_SHAKE_PULSE, INIT_SHAKE_CNT);
        CONTROL_MODE = CONTROL_MODE_FOC_HOME_CONTROL;
        InitControlState = 11;        
        break;
        
    case 11:
        if(home_param.move_flag == 0){
            if(abs(init_start_pos+INIT_SHAKE_PULSE-currentMotorPosition) > INIT_SHAKE_PULSE/4){
                init_enc_error_cnt++;
            }
            sole_shake_push_cnt = 500;
            solenoid_stat = SOLENOID_STAT_SHAKE_PUSH;
            set_home_start_pos(home_param.cur_pos);
            set_home_target(init_start_pos-INIT_SHAKE_PULSE, INIT_SHAKE_CNT*2);
            InitControlState = 12;
        }
        break;
        
    case 12:
        if(home_param.move_flag == 0){
            if(abs(init_start_pos-INIT_SHAKE_PULSE-currentMotorPosition) > INIT_SHAKE_PULSE/4){
                init_enc_error_cnt++;
            }
            sole_shake_push_cnt = 500;
            solenoid_stat = SOLENOID_STAT_SHAKE_PUSH;                
            set_home_start_pos(home_param.cur_pos);
            set_home_target(init_start_pos, INIT_SHAKE_CNT);
            InitControlState = 13;
        }
        break;
        
    case 13:
        if(home_param.move_flag == 0){
            if(init_enc_error_cnt > 0){
                //STAT.b.INIT_ENC = 1;
            }
            STAT.b.INIT = 1;
            InitControlState = 0;
            InitControlFlag = 0;
            set_goto_start_pos(init_start_pos);
            NewInputRefAbs = goto_param.cur_pos = init_start_pos;
            CONTROL_MODE = CONTROL_MODE_FOC_POS_CONTROL;
        }
        break;
    }
}
