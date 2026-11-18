#include <proc/p32mk1024mcf064.h>

#include "init.h"
#include "board_definition.h"
#include "timer.h"
#include "port.h"
#include "can.h"
#include "adc.h"
#include "qei.h"
#include "spi.h"
#include "status.h"
#include "control.h"
#include "execution.h"
#include "dac.h"
#include "FOC.h"
#include "home.h"
#include "SV_table.h"


static void InitParameters();
static void GetROM();
static void ROMData();
extern unsigned int _SPI_WRITE(unsigned int data);


void Handler_Init(){
    if(INIT_BOARD_MODE == INIT_BOARD_MODE_RESTORE){
        InitBoardIdentification();
    }
}


int id_list[5];
int board_list[5];
int temp_list[5];
void InitBoardIdentification(){
    int board_type;
    int temper;
    int i, j;
    
    STAT.B = 0;
    
    TRISBbits.TRISB5 = 0;   // MU150_CS
    
    EEPROM_DESELECT
    MU150_DESELECT
    MU150_DESELECT2
//    ACC_DESELECT
        
   
    //InitSPI(8, 2);  // 8bit, high idle - falling edge
    InitSPI(8, 1);
    for(i=0;i<10000;i++)
        ;
    
    for(i=0; i<3; i++){
        board_type = EEPROM_READ(MAX_CH, EE_COMMON_BOARDTYPE);
        JMC = EEPROM_READ(MAX_CH, EE_COMMON_JMC);
        temper = EEPROM_READ(MAX_CH, EE_COMMON_TEMP_ERROR);
        

        if(JMC == 0xFF){
            //INIT_BOARD_MODE = INIT_BOARD_MODE_RESET_BTN;
        }else{
            break;
        }
    }
    //uiuk modified
    
    if(JMC > 14 || JMC < 0){//== 0xFF && board_type == 0xFF){
        INIT_BOARD_MODE = INIT_BOARD_MODE_RESET_BTN;
        JMC = 12;
    }
    
//    if(JMC > 11 || JMC < 0){//== 0xFF && board_type == 0xFF){
//        INIT_BOARD_MODE = INIT_BOARD_MODE_RESET_BTN;
//        JMC = 6;
//    }


    // first flashing program -----
//    INIT_BOARD_MODE = INIT_BOARD_MODE_RESET_BTN;
//    JMC = 3;
    // ----------------------------
    
    if(INIT_BOARD_MODE == INIT_BOARD_MODE_RESET_BTN){
        InitROM(0);
        
        INIT_BOARD_MODE = INIT_BOARD_MODE_OK;
        for(i=0; i<1000; ++i)
			for(j=0; j<500; ++j)
                ;
        //return;
    }
    

    // == Initialize Peripherals & Parameters =====   
    InitPort();
	InitParameters();
 
	GetROM(); 
	ROMData();
  
    for(i=0; i<1000; ++i)
        for(j=0; j<500; ++j)
            ;
}

void InitPeripheral(){
    InitCANID(JMC);
    InitCAN();
    InitADC();
    InitQEI();
//    InitTimer2();
//    SuspendTimer2();
    InitTimer3();
    InitTimer4();
    InitPWM();
}

static void InitParameters(){
    ERROR_sum = 0.0;

    // Initialize Status and Control Bytes
    STAT.B = 0;		

    JAM_cnt = 0;
    OVR_cnt = 0;
	Initialize_MotorParameter();
}


void InitROM(int mode){   
    if(mode == 0){
        // Write Directly
        EEPROM_WRITE(MAX_CH, EE_COMMON_JMC, JMC);
        EEPROM_WRITE(MAX_CH, EE_COMMON_BOARDTYPE, BTY_1CH);
        EEPROM_WRITE(MAX_CH, EE_COMMON_JMC_NOT, ~(JMC&0xFF));
        EEPROM_WRITE(MAX_CH, EE_COMMON_TEMP_ERROR, DEFAULT_TEMP_ERROR);
    
    
        EEPROM_WRITE(0, EE_TYPE_ENC_RES, DEFAULT_ENC_RES);
        //EEPROM_WRITE(0, EE_TYPE_POLE_NUM, DEFAULT_POLE_NUM);
        EEPROM_WRITE(0, EE_TYPE_AUTO_SCALE, 0);
        if(JMC == 0 || JMC == 2 || JMC == 4){
            EEPROM_WRITE(0, EE_TYPE_MOTOR_DIR, 1);
        }else{
            EEPROM_WRITE(0, EE_TYPE_MOTOR_DIR, 0);
        }



        EEPROM_WRITE(0, EE_TYPE_JAM_LIMIT_CURRENT, DEFAULT_JAM_LIMIT_CURRENT);
        EEPROM_WRITE(0, EE_TYPE_OVR_LIMIT_CURRENT, DEFAULT_OVR_LIMIT_CURRENT);
        EEPROM_WRITE(0, EE_TYPE_JAM_LIMIT_TIME, DEFAULT_JAM_LIMIT_TIME);
        EEPROM_WRITE(0, EE_TYPE_OVR_LIMIT_TIME, DEFAULT_OVR_LIMIT_TIME);
        EEPROM_WRITE(0, EE_TYPE_INPUT_ERROR, DEFAULT_INPUT_ERROR);
        EEPROM_WRITE(0, EE_TYPE_BIG_ERROR, DEFAULT_BIG_ERROR);
        EEPROM_WRITE(0, EE_TYPE_POS_LOWER_LIMIT, DEFAULT_POS_LOWER_LIMIT);
        EEPROM_WRITE(0, EE_TYPE_POS_UPPER_LIMIT, DEFAULT_POS_UPPER_LIMIT);


        EEPROM_WRITE(0, EE_TYPE_FOC_PGAIN, DEFAULT_FOC_PGAIN);
        EEPROM_WRITE(0, EE_TYPE_FOC_IGAIN, DEFAULT_FOC_IGAIN);
        EEPROM_WRITE(0, EE_TYPE_POS_PGAIN, DEFAULT_POS_PGAIN);
        EEPROM_WRITE(0, EE_TYPE_POS_IGAIN, DEFAULT_POS_IGAIN);
        EEPROM_WRITE(0, EE_TYPE_POS_DGAIN, DEFAULT_POS_DGAIN);


        EEPROM_WRITE(0, EE_TYPE_CURRENT_NULL_RZ, DEFAULT_CURRENT_NULL_RZ);
        EEPROM_WRITE(0, EE_TYPE_CURRENT_NULL_SZ, DEFAULT_CURRENT_NULL_SZ);
        EEPROM_WRITE(0, EE_TYPE_CURRENT_NULL_TZ, DEFAULT_CURRENT_NULL_TZ);


        EEPROM_WRITE(0, EE_TYPE_HOME_ZERO_OFFSET, 0);
        EEPROM_WRITE(0, EE_TYPE_MU_DISK_OFFSET, 0);
    }else{
         // Write to Buffer
        Push_EEPROM_Ring(MAX_CH, EE_COMMON_JMC, JMC);
        Push_EEPROM_Ring(MAX_CH, EE_COMMON_BOARDTYPE, BTY_1CH);
        Push_EEPROM_Ring(MAX_CH, EE_COMMON_JMC_NOT, ~(JMC&0xFF));
        Push_EEPROM_Ring(MAX_CH, EE_COMMON_TEMP_ERROR, DEFAULT_TEMP_ERROR);
    
    
        Push_EEPROM_Ring(0, EE_TYPE_ENC_RES, DEFAULT_ENC_RES);
        //EEPROM_WRITE(0, EE_TYPE_POLE_NUM, DEFAULT_POLE_NUM);
        Push_EEPROM_Ring(0, EE_TYPE_AUTO_SCALE, 0);
        if(JMC == 0 || JMC == 2 || JMC == 4){
            Push_EEPROM_Ring(0, EE_TYPE_MOTOR_DIR, 1);
        }else{
            Push_EEPROM_Ring(0, EE_TYPE_MOTOR_DIR, 0);
        }



        Push_EEPROM_Ring(0, EE_TYPE_JAM_LIMIT_CURRENT, DEFAULT_JAM_LIMIT_CURRENT);
        Push_EEPROM_Ring(0, EE_TYPE_OVR_LIMIT_CURRENT, DEFAULT_OVR_LIMIT_CURRENT);
        Push_EEPROM_Ring(0, EE_TYPE_JAM_LIMIT_TIME, DEFAULT_JAM_LIMIT_TIME);
        Push_EEPROM_Ring(0, EE_TYPE_OVR_LIMIT_TIME, DEFAULT_OVR_LIMIT_TIME);
        Push_EEPROM_Ring(0, EE_TYPE_INPUT_ERROR, DEFAULT_INPUT_ERROR);
        Push_EEPROM_Ring(0, EE_TYPE_BIG_ERROR, DEFAULT_BIG_ERROR);
        Push_EEPROM_Ring(0, EE_TYPE_POS_LOWER_LIMIT, DEFAULT_POS_LOWER_LIMIT);
        Push_EEPROM_Ring(0, EE_TYPE_POS_UPPER_LIMIT, DEFAULT_POS_UPPER_LIMIT);


        Push_EEPROM_Ring(0, EE_TYPE_FOC_PGAIN, DEFAULT_FOC_PGAIN);
        Push_EEPROM_Ring(0, EE_TYPE_FOC_IGAIN, DEFAULT_FOC_IGAIN);
        Push_EEPROM_Ring(0, EE_TYPE_POS_PGAIN, DEFAULT_POS_PGAIN);
        Push_EEPROM_Ring(0, EE_TYPE_POS_IGAIN, DEFAULT_POS_IGAIN);
        Push_EEPROM_Ring(0, EE_TYPE_POS_DGAIN, DEFAULT_POS_DGAIN);


        Push_EEPROM_Ring(0, EE_TYPE_CURRENT_NULL_RZ, DEFAULT_CURRENT_NULL_RZ);
        Push_EEPROM_Ring(0, EE_TYPE_CURRENT_NULL_SZ, DEFAULT_CURRENT_NULL_SZ);
        Push_EEPROM_Ring(0, EE_TYPE_CURRENT_NULL_TZ, DEFAULT_CURRENT_NULL_TZ);


        Push_EEPROM_Ring(0, EE_TYPE_HOME_ZERO_OFFSET, 0);
        Push_EEPROM_Ring(0, EE_TYPE_MU_DISK_OFFSET, 0);
    }
}


static void GetROM(){
    T_ERR = EEPROM_READ(MAX_CH, EE_COMMON_TEMP_ERROR);
    
    AUTO_SCALE_FLAG = EEPROM_READ(0, EE_TYPE_AUTO_SCALE);
    Encoder_res = EEPROM_READ(0, EE_TYPE_ENC_RES);
    Pole_number = EEPROM_READ(0, EE_TYPE_POLE_NUM);
    //MDR = EEPROM_READ(0, EE_TYPE_MOTOR_DIR);
    HomeDir = EEPROM_READ(0, EE_TYPE_MOTOR_DIR);
    
    if(JMC == 0 || JMC == 2 || JMC == 4){
        MDR = 1;
    }else{
        MDR = 0;
    }

    
    JAM_LIM_Current = EEPROM_READ(0, EE_TYPE_JAM_LIMIT_CURRENT);
    OVR_LIM_Current = EEPROM_READ(0, EE_TYPE_OVR_LIMIT_CURRENT);
    JAM_LIM_Time = EEPROM_READ(0, EE_TYPE_JAM_LIMIT_TIME);
    OVR_LIM_Time = EEPROM_READ(0, EE_TYPE_OVR_LIMIT_TIME);
    I_ERR = EEPROM_READ(0, EE_TYPE_INPUT_ERROR);
    B_ERR = EEPROM_READ(0, EE_TYPE_BIG_ERROR);
    POS_LOWER_LIM = EEPROM_READ(0, EE_TYPE_POS_LOWER_LIMIT);
    POS_UPPER_LIM = EEPROM_READ(0, EE_TYPE_POS_UPPER_LIMIT);
    
    
    CUR_BIG_Time = 10;  // 1sec
    CUR_BIG_LIM = 5000; // 5A
    
    OVER_CURRENT = OVR_LIM_Current * 8 / 10;
    OVER_CURRENT_TIME = 20; // 20ms
    

    if(POS_LOWER_LIM & 0x01){
        MAX_POS1_flag = 1;
    }else{
        MAX_POS1_flag = 0;
    }
    
    if(POS_UPPER_LIM & 0x01){
        MAX_POS2_flag = 1;
    }else{
        MAX_POS2_flag = 0;
    }
    
    
    

    if(I_ERR > 0xFFFF)
        I_ERR=0xFFFF;
    if(B_ERR > 0xFFFF)
        B_ERR=0xFFFF;

    
    
    Pgain_d1000 = Pgain_q1000 = EEPROM_READ(0, EE_TYPE_FOC_PGAIN);
    Igain_d1000 = Igain_q1000 = EEPROM_READ(0, EE_TYPE_FOC_IGAIN);
    
    Pgain_pos = EEPROM_READ(0, EE_TYPE_POS_PGAIN);
    Igain_pos = EEPROM_READ(0, EE_TYPE_POS_IGAIN);
    Dgain_pos = EEPROM_READ(0, EE_TYPE_POS_DGAIN);
    
    
    RZ = EEPROM_READ(0, EE_TYPE_CURRENT_NULL_RZ);
    SZ = EEPROM_READ(0, EE_TYPE_CURRENT_NULL_SZ);
    TZ = EEPROM_READ(0, EE_TYPE_CURRENT_NULL_TZ);
    
    
    HOME_ZERO_OFFSET = EEPROM_READ(0, EE_TYPE_HOME_ZERO_OFFSET);
    ABS_OFFSET_ENC = 0;//-HOME_ZERO_OFFSET;
    
    MU_DISK_OFFSET = EEPROM_READ(0, EE_TYPE_MU_DISK_OFFSET);
       
    InitPos_DQ_MU = EEPROM_READ(0, EE_TYPE_INIT_DQ_MU_POS);
}


static void ROMData(){
    RomData[MAX_CH][EE_COMMON_JMC] = JMC;
    RomData[MAX_CH][EE_COMMON_BOARDTYPE] = BTY_1CH;
    RomData[MAX_CH][EE_COMMON_TEMP_ERROR] = T_ERR;
      
    RomData[0][EE_TYPE_ENC_RES] = Encoder_res;
    RomData[0][EE_TYPE_MOTOR_DIR] = MDR;

    RomData[0][EE_TYPE_JAM_LIMIT_CURRENT] = JAM_LIM_Current;
    RomData[0][EE_TYPE_OVR_LIMIT_CURRENT] = OVR_LIM_Current;
    RomData[0][EE_TYPE_JAM_LIMIT_TIME] = JAM_LIM_Time;
    RomData[0][EE_TYPE_OVR_LIMIT_TIME] = OVR_LIM_Time;
    RomData[0][EE_TYPE_INPUT_ERROR] = I_ERR;
    RomData[0][EE_TYPE_BIG_ERROR] = B_ERR;
    RomData[0][EE_TYPE_POS_LOWER_LIMIT] = POS_LOWER_LIM;
    RomData[0][EE_TYPE_POS_UPPER_LIMIT] = POS_UPPER_LIM;
}
