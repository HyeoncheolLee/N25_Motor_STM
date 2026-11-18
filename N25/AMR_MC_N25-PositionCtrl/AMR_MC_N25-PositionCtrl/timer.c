#include <proc/p32mk1024mcf064.h>
#include <sys/attribs.h>

#include "timer.h"
#include "qei.h"
#include "adc.h"
#include "SV_table.h"


void InitTimer4(){
    T4CON = 0x0000;
    T4CONbits.T32 = 1;
    PR4 = 0xFFFFFFFF;
    TMR4 = 0;
    T4CONbits.ON = 1;
}


void InitTimer2(){
    T2CON = 0x0000;                 // clear
//    T2CONbits.TCKPS = 0b101;        // prescale 32:1
//    T2CONbits.TCKPS = 0b001;        // prescale 2:1
    T2CONbits.TCKPS = 0;            // prescale 1:1
    T2CONbits.TCS = 0;              // internal clock
    TMR2 = 0;
//    PR2 = 1875-1;                   // 60,000,000 / 32 = 1,875,000 --> 1,875,000 / 1,000 = 1,875
//    PR2 = 30000-1;                  // 60,000,000 / 2 = 30,000,000 --> 30,000,000 / 1,000 = 30,000
    PR2 = 60000-1;
    
    IPC2bits.T2IP = 4;               // priority
    IPC2bits.T2IS = 0;              // sub priority
    IFS0bits.T2IF = 0;              // clear the flag
    
    T2CONbits.ON = 1;               // on TMR 2
    IEC0bits.T2IE = 1;              // enable interrupt
}

void InitTimer3(){
    TMR3 = 0x0000;
    PR3 = 3000-1;

    IPC3bits.T3IP = 6;              // priority
    IPC3bits.T3IS = 2;              // sub priority
    IFS0bits.T3IF = 0;
    
    T3CONbits.ON = 1;
    IEC0bits.T3IE = 1;
}



void SuspendTimer2(){
    T2CONbits.ON = 0;
}

void ResumeTimer2(){
    T2CONbits.ON = 1;
}

void InitSystemClks(){
    SYSKEY = 0x0;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
//    PRECONbits.PFMWS=2;
//    PRECONbits.PFMSECEN=0;
//    PRECONbits.PREFEN=0x1;

    while( !PB1DIVbits.PBDIVRDY);
    PB1DIVbits.PBDIV = 0;

    //PB2 = src for PMP, I2C, UART, SPI
    PB2DIVbits.ON = 1;
    while( !PB2DIVbits.PBDIVRDY);
    PB2DIVbits.PBDIV = 1;

    //PB3 = src for ADC, timers, input capture, output capture, comparator
    PB3DIVbits.ON = 1;
    while( !PB3DIVbits.PBDIVRDY);
    PB3DIVbits.PBDIV = 1;

    PB4DIVbits.ON = 1;
    while( !PB4DIVbits.PBDIVRDY);
    PB4DIVbits.PBDIV = 1;

    PB5DIVbits.ON = 1;
    while( !PB5DIVbits.PBDIVRDY);
    PB5DIVbits.PBDIV = 1;

    PB7DIVbits.ON = 1;
    while( !PB7DIVbits.PBDIVRDY);
    PB7DIVbits.PBDIV = 1;

    SYSKEY = 0x0;
}


