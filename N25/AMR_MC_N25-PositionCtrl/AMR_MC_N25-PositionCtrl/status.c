#include <proc/p32mk1024mcf064.h>
#include <math.h>

#include "status.h"
#include "can.h"
#include "adc.h"
#include "qei.h"
#include "control.h"


#define MOTOR_FAULT    PORTEbits.RE13

int     JMC;
int     INIT_BOARD_MODE = INIT_BOARD_MODE_OK;
char    BoardName[13] = "MCF064_AC";


mSTAT   STAT;
int     STATUS_FLAG = 0;



// error --------------------
int T_ERR;
long long I_ERR, B_ERR;
unsigned long long JAM_cnt = 0, OVR_cnt = 0, CUR_BIG_cnt = 0, R_cnt = 0, S_cnt = 0, T_cnt = 0;
unsigned long long JAM_LIM_Current, OVR_LIM_Current, JAM_LIM_Time, OVR_LIM_Time, CUR_BIG_LIM, CUR_BIG_Time = 10;





static void SendStatus(){
 	unsigned int dlc;
 	unsigned char canTxData[8] = {0,};
    unsigned char temp_board = temperature_board_celsius_x10/10;
    unsigned char temp_motor = temperature_motor_celsius_x10/10;
    
	canTxData[0] = (unsigned char)(STAT.B&0xFF);		// STAT Byte
	canTxData[1] = (unsigned char)((STAT.B>>8)&0xFF);	// STAT Byte
	canTxData[2] = (unsigned char)((STAT.B>>16)&0xFF);	// STAT Byte
    canTxData[3] = (unsigned char)(temp_board);
    canTxData[4] = (unsigned char)(temp_motor);
    dlc = 5;
	
	Push_CAN_TX_Ring(CANID_STAT, dlc, canTxData);
}

void Handler_Status(unsigned long cnt){
    static unsigned long prev_cnt = 0;
    static unsigned int old_stat = 0;
    static unsigned int error_report_cnt = 0;
//    static unsigned int periodic_send_cnt = 0;
    
    if(old_stat != STAT.B){
        STATUS_FLAG = 1;
    }
    old_stat = STAT.B;
    
    if(cnt - prev_cnt > 0){
        prev_cnt = cnt;
        error_report_cnt++;
//        periodic_send_cnt++;
        if(error_report_cnt > 1000){
            error_report_cnt = 0;

            if((STAT.B&0x0000FF00) != 0)
                error_report_cnt++;

//            if(STAT.b.HME != 6 && STAT.b.FET == 1 && STAT.b.RUN == 1)
//                error_report_cnt++;
            
            if(error_report_cnt > 0)
                SET_STATUS_FLAG
            error_report_cnt = 0;
        }
        
        
//        if(periodic_send_cnt > 1000){
//            periodic_send_cnt = 0;
//            SET_STATUS_FLAG
//        }
    }
    
    if(STATUS_FLAG == 1){
        SendStatus();
        STATUS_FLAG = 0;
    }
}


void Handler_Error(unsigned long cnt){
    static unsigned long prev_cnt = 0;
    static unsigned int temperature_cnt = 0;
    static unsigned int fault_error_cnt = 0;
    int tick_passed = 0;
    
    
    if(cnt - prev_cnt > 0){
        prev_cnt = cnt;
        tick_passed = 1;
    }
    
    
    // ERROR - motor fault -----------------------------
    if(MOTOR_FAULT == 0){
        STAT.b.FLT = 1;
        fault_error_cnt = 30;
    }else{
        fault_error_cnt = 0;
    }
		
	
    // from here -- more than one tick has been passed -----------------------------------------------
    if(tick_passed == 0)
        return;
        
    
    // ERROR - motor fault -----------------------------
    if(fault_error_cnt > 0){
        --fault_error_cnt;
    }else if(fault_error_cnt == 0){
        if(STAT.b.FLT == 1){
            STAT.b.FLT = 0;
        }
    }
    
    // ERROR - temperature high -----------------------------
    temperature_cnt++;
    if(temperature_cnt > 1000){
        temperature_cnt = 0;
        
        // temperature error
        if(temperature_board_celsius_x10 > T_ERR*10){
			STAT.b.TMP = 1;
        }else{
            STAT.b.TMP = 0;
        }
    }
}