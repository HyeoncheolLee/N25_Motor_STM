#include <proc/p32mk1024mcf064.h>
#include <sys/attribs.h>

#include "port.h"


void InitPort(){
    ANSELEbits.ANSE13 = 0;
    TRISEbits.TRISE13 = 0;  // LED_STATUS
    ANSELAbits.ANSA12 = 0;
    TRISAbits.TRISA12 = 0;  // LED_STAT
    
    // MOTOR Enable & Fault
//    TRISAbits.TRISA7 = 0;
//    ANSELEbits.ANSE13 = 0;
//    TRISEbits.TRISE13 = 1;
    TRISAbits.TRISA11 = 0;
    ANSELAbits.ANSA11 = 0;
    
    // SolGnd
//    ANSELAbits.ANSA11 = 0;
//    TRISAbits.TRISA11 = 1;//0;
    
    
    
    TRISAbits.TRISA8 = 1;
    TRISAbits.TRISA4 = 1;
    TRISAbits.TRISA10 = 1;
    //TRISAbits.TRISA11 = 1;
    TRISBbits.TRISB0 = 1;
    TRISBbits.TRISB4 = 1;
    TRISBbits.TRISB8 = 1;
    TRISCbits.TRISC6 = 1;
    TRISCbits.TRISC8 = 1;
    TRISCbits.TRISC10 = 1;
    TRISCbits.TRISC11 = 1;
    TRISCbits.TRISC12 = 1;
    TRISCbits.TRISC13 = 1;
    TRISCbits.TRISC15 = 1;
    TRISDbits.TRISD6 = 1;
    TRISDbits.TRISD8 = 1;
    TRISGbits.TRISG9 = 1;
//    TRISFbits.TRISF0 = 1;
    TRISFbits.TRISF1 = 1;
    
    
}
