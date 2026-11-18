#include <proc/p32mk1024mcf064.h>

#include "board_definition.h"
#include "pwm.h"

int solenoid_stat = SOLENOID_STAT_TURN_OFF;
int sole_hard_push_cnt = 0;
int sole_shake_push_cnt = 0;
int sole_weak_cnt = 0;

int sole_weak_push_duty = 0;

void InitPWM(){
    PMD4bits.PWM4MD = 1;
    PMD4bits.PWM5MD = 0;
    PMD4bits.PWM6MD = 1;
    PMD4bits.PWM11MD = 1;
    
    TRISBbits.TRISB15 = 0;  // PWM L1
    TRISBbits.TRISB14 = 0;  // PWM H1
    TRISBbits.TRISB13 = 0;  // PWM L2
    TRISBbits.TRISB12 = 0;  // PWM H2
    TRISBbits.TRISB11 = 0;  // PWM L3
    TRISBbits.TRISB10 = 0;  // PWM H3
    TRISCbits.TRISC9 = 0;   // SOL_GND - PWM L5
    
    // PTPER = (FSYSCLK / (FPWM *(2^PTCON<PCLKDIV>)))
    // FPWM = 40kHz
//    PTCONbits.PCLKDIV = 0;
//    PTPER = 3000;
    
    // FPWM = 80kHz
    PTCONbits.PCLKDIV = 0;
    PTPER = PWM_RES;
    
    PWMCON1bits.ECAM = 0b01;    // edge-aligned mode
    PWMCON2bits.ECAM = 0b01;    // edge-aligned mode
    PWMCON3bits.ECAM = 0b01;    // edge-aligned mode
    PWMCON4bits.ECAM = 0b00;    // edge-aligned mode
        

    IOCON1bits.PMOD = 0b00;     // complementary
    IOCON1bits.PENH = 1;        // PWM (not GPIO)
    IOCON1bits.PENL = 1;        // PWM (not GPIO)
    IOCON1bits.FLTMOD = 0b11;   // fault disable
    IOCON1bits.POLH = 1;        // Active Low
    IOCON1bits.POLL = 1;        // Active Low
 
    IOCON2bits.PMOD = 0b00;     // complementary
    IOCON2bits.PENH = 1;        // PWM (not GPIO)
    IOCON2bits.PENL = 1;        // PWM (not GPIO)
    IOCON2bits.FLTMOD = 0b11;   // fault disable
    IOCON2bits.POLH = 1;        // Active Low
    IOCON2bits.POLL = 1;        // Active Low
   
    IOCON3bits.PMOD = 0b00;     // complementary
    IOCON3bits.PENH = 1;        // PWM (not GPIO)
    IOCON3bits.PENL = 1;        // PWM (not GPIO)
    IOCON3bits.FLTMOD = 0b11;   // fault disable
    IOCON3bits.POLH = 1;        // Active Low
    IOCON3bits.POLL = 1;        // Active Low


    
    IOCON5bits.PMOD = 0b00;     // complementary
    IOCON5bits.PENH = 0;        // PWM (not GPIO) -- 
    IOCON5bits.PENL = 1;        // PWM (not GPIO) -- SOL_GND
    IOCON5bits.FLTMOD = 0b11;   // fault disable
    IOCON5bits.POLH = 0;        // Active High
    IOCON5bits.POLL = 1;        // Active Low
    IOCON5bits.OVRENH = 0;      // 
    IOCON5bits.OVRENL = 0;      // for SOL_GND - pulse
    
    
    IOCON1bits.OVRENH = 0;  // pwm      - pulse
    IOCON1bits.OVRENL = 0;  // pwm      - pulse
    IOCON2bits.OVRENH = 0;  // pwm      - pulse
    IOCON2bits.OVRENL = 0;  // pwm      - pulse
    IOCON3bits.OVRENH = 0;  // pwm      - pulse
    IOCON3bits.OVRENL = 0;  // pwm      - pulse
    
    IOCON1bits.OVRDAT = 0b00;
    IOCON2bits.OVRDAT = 0b00;
    IOCON3bits.OVRDAT = 0b00;
    
    
    DTR1 = 40;          // 8*40 = 320ns
    DTR2 = 40;          // 8*40 = 320ns
    DTR3 = 40;          // 8*40 = 320ns
    ALTDTR1 = 40;       // 8*40 = 320ns
    ALTDTR2 = 40;       // 8*40 = 320ns
    ALTDTR3 = 40;       // 8*40 = 320ns
    
    PTCONbits.PTEN = 1;
    TMR3 = 750;
    
#ifdef NEW_AMP2
    PDC1 = PWM_RES;
    PDC2 = PWM_RES;
    PDC3 = PWM_RES; 
#else
    PDC1 = 0;
    PDC2 = 0;
    PDC3 = 0; 
#endif
    PDC5 = 0;
}

void Handler_Solenoid(){
    if(solenoid_stat == SOLENOID_STAT_HARD_PUSH){
//        if(sole_hard_push_cnt < SOLENOID_HARD_PUSH_COUNT/2){
//            PDC5 = PWM_RES;//3000;
//        }else{
//            //PDC5 = PWM_RES*2*(SOLENOID_HARD_PUSH_COUNT-sole_hard_push_cnt)/SOLENOID_HARD_PUSH_COUNT;
//            PDC5 = PWM_RES*3/4;
//        }
        PDC5 = PWM_RES*50/100;
        
        if(sole_hard_push_cnt-- < 0){
            sole_weak_cnt = 0;
            solenoid_stat = SOLENOID_STAT_WEAK_PUSH;
        }
    }else if(solenoid_stat == SOLENOID_STAT_WEAK_PUSH){
        //PDC5 = PWM_RES*15/100;//450;
        //PDC5 = PWM_RES*8/100;//450;
        
//        PDC5 = sole_weak_push_duty;
        PDC5 = PWM_RES*23/100;
//#ifdef BOARD_TYPE_25
//        PDC5 = PWM_RES*12/100;
//#else
//        PDC5 = PWM_RES*5/100;
//#endif
        sole_weak_cnt++;
//        if(sole_weak_cnt-- < 0){
//            sole_hard_push_cnt = SOLENOID_HARD_PUSH_COUNT;
//            solenoid_stat = SOLENOID_STAT_HARD_PUSH;
//        }
    }else if(solenoid_stat == SOLENOID_STAT_TURN_OFF){
        PDC5 = 0;
    }else if(solenoid_stat == SOLENOID_STAT_SHAKE_PUSH){
        PDC5 = PWM_RES/2;
        
        if(sole_shake_push_cnt-- < 0){
            sole_weak_cnt = 0;
            solenoid_stat = SOLENOID_STAT_WEAK_PUSH;
        }
    }
}

