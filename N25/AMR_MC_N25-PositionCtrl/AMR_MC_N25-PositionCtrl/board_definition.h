#ifndef __BOARD_DEFINITION_H__
#define __BOARD_DEFINITION_H__

// Version Information---------------
//#define VERSION     1230912         // First distributed version
//#define VERSION     1231208         // InitRom refined
//#define VERSION     1240215         // AMR - Hubo-i encoder rate reduce || PWM Freq 40kHz -> 20kHz
//#define VERSION     1240229         // AMR - Low Diff - Lift & Turn Code
//#define VERSION     1240318         // AMR - Low Diff Tank Motion
//#define VERSION     1240508         // AMR - 3 Current Sensor
//#define VERSION     1240524         // AMR - 7 pole pair
//#define VERSION     1240614         // AMR - init shaking for brake release
//#define VERSION     1240919         // AMR - Dual Encoder
//#define VERSION     1250621         // AMR - Speed Control
//#define VERSION     1260824         // N25 Position Control Idle State Current Taper(Donghae N32 Test ver Merge)
#define VERSION     1260915         // Big Error Reduction & Increase in current recovery speed 10A/s → 20A/s
//-----------------------------------

#define CURRENT_REMOVE



#define NO_NEW_FUNCTION

#define GAIN_P_CONSTANT
#define NEW_AMP2 




//    #define NO_OF_POLE          5
//    #define ENC_POLE_VAL        3276.8
#define NO_OF_POLE          7 
#define ENC_POLE_VAL        2340.571429


#define MUL     10


// CURRENT_CONSTANT -----------------    
//     3.7/2.7   *   3.3/4096     *     10     *   1000  =  11.04058 (mA/digit)
//     v-divider     volt/adc  sensitivity(100mV/A) mA
    // I * R * G * (R1/R2+R1) = V   --->   I = V/R/G/(R1/R2+R1)
    // (3.3/4096) / 0.001 / 20 = 40.2832 (1mOhm)
    // (3.3/4096) / 0.002 / 20 = 20.1416 (2mOhm)
    // (3.3/4096) / 0.003 / 20 = 13.4277 (3mOhm)
    // (3.3/4096) / 0.005 / 20 = 8.0566  (5mOhm)
//#define CURRENT_CONSTANT    13.4277       // serving robot
#define CURRENT_CONSTANT    20.1416
//-----------------------------------


#define ENC_RES                     16384
#define MAX_CUR_LIMIT               15000   //16000


#define DEFAULT_FOC_PGAIN           700     // MOBILE SPECIFIC
#define DEFAULT_FOC_IGAIN           10      // MOBILE SPECIFIC

#define DEFAULT_POS_PGAIN           70      // 60      //70      // MOBILE SPECIFIC
#define DEFAULT_POS_IGAIN           0       // MOBILE SPECIFIC
#define DEFAULT_POS_DGAIN           1000    // 2000    //4000    // MOBILE SPECIFIC

#define DEFAULT_JAM_LIMIT_CURRENT   25000   //13000   // 
#define DEFAULT_OVR_LIMIT_CURRENT   27000   // 

#define DEFAULT_JAM_LIMIT_TIME      8      // 0.8sec
#define DEFAULT_OVR_LIMIT_TIME      8      // 0.8sec
#define DEFAULT_INPUT_ERROR         3000
#define DEFAULT_BIG_ERROR           65535


// ==============================================

#define MAX_CH      2

#define PI          3.14159265359
#define R2D         57.29577951
#define D2R         0.017453292

#define CURRENT_CONTROL

#define ENABLE			1
#define DISABLE			0

#define TRUE    1
#define FALSE   0

#define	CTRL_MOD_POSITION		0
#define	CTRL_MOD_CURRENT		1


#define MOTOR_HIP_ON   LATAbits.LATA11 = 1;
#define MOTOR_HIP_OFF  LATAbits.LATA11 = 0;

#define PRESET_ON       LATBbits.LATB3 = 1;
#define PRESET_OFF      LATBbits.LATB3 = 0;



// CONTROL MODE--------------------
#define CONTROL_MODE_NO_ACT                 0
#define CONTROL_MODE_FOC_OPENLOOP_CONTROL   1
#define CONTROL_MODE_FOC_CUR_CONTROL        2
#define CONTROL_MODE_FOC_POS_CONTROL        3
#define CONTROL_MODE_FOC_HOME_CONTROL       4

#define CONTROL_MODE_STO                    5
#define CONTROL_MODE_SS1_POS                6
#define CONTROL_MODE_SS2_POS                7
// --------------------------------


typedef union{
	struct {
		unsigned    FET:1;	 	// FET ON
		unsigned    RUN:1;		// Control ON
        unsigned    INIT:1;     // Init Process Passed
		unsigned    MOD:1;		// Control Mode
        unsigned    NON_CTR:1;  // Nonius Error
		unsigned 	BAT:1;		// Low Battery
        unsigned    CALIB:1;    // Calibration Mode
        unsigned    MT_ERR:1;   // Multi-Turn Error
    	
    	unsigned 	JAM:1;		// JAM Error
      	unsigned 	CUR:1;		// Over Current Error
		unsigned    BIG:1;		// Big Position Error		
		unsigned    INP:1;	    // Big Input Error
		unsigned 	FLT:1;		// FET Driver Fault Error
		unsigned    TMP:1;	    // Temperature Error		
		unsigned	PS1:1;		// Position Limit Error (Lower)
		unsigned    PS2:1;		// Position Limit Error (Upper)
		
        unsigned    SPI_EXT:1;  // External SPI
		unsigned    CUR_BIG:1;  // Current Big
        unsigned    CAN_ERR:1;  // CAN_ERR in Current Mode
		unsigned	rsvd:13;
		
	} b;
	unsigned int B;
} mSTAT;



#endif
