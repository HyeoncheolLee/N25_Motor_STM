#include <proc/p32mk1024mcf064.h>
#include <string.h>

#include "spi.h"


#define seokgyun_debug

// EEPROM Instruction
#define		WRITE_ENABLE		0x06
#define		WRITE_DISABLE		0x04
#define		READ_STAT_REG		0x05
#define		WRITE_STAT_REG		0x01
#define		READ_MEMORY         0x03
#define		WRITE_MEMORY		0x02

// EEPROM Status Register Format
#define		SRWD				0x80	// Status Register Write Disable bit
#define		BP1					0x08	// Block Protect bit
#define		BP0					0x04	// Block Protect bit
#define		WEL					0x02	// Write Enable Latch bit
#define		WIP					0x01	// Write In Progress bit

#define     EEPROM_BUFF_SIZE    30

typedef struct{
    unsigned int head;
    unsigned int tail;
	unsigned int ch[EEPROM_BUFF_SIZE];
	unsigned int type[EEPROM_BUFF_SIZE];
    unsigned int data[EEPROM_BUFF_SIZE];
} EEPROM_RING;
static EEPROM_RING _eeprom_ring;


unsigned int RomData[MAX_CH+1][EE_TYPE_MAX];


static unsigned int _EEPROM_ReadStatus();
unsigned int _SPI_WRITE(unsigned int data);

static void _Init_EEPROM_Ring();



#define MU_BUFF_SIZE    100
typedef struct{
	unsigned int head;
	unsigned int tail;
	unsigned int addr[MU_BUFF_SIZE];
	unsigned int data[MU_BUFF_SIZE];
} MU_RING;
static MU_RING _mu_ring;



extern int JMC;


void InitSPI(unsigned int data_bit, unsigned int mode){
    int dummy;
    char bit_mode[2];
    char mode_select[2];
    
    _Init_EEPROM_Ring();
    Init_MU_Ring();
    
    ANSELEbits.ANSE12 = 0;
    ANSELFbits.ANSF0 = 0;
    TRISEbits.TRISE12 = 0;  // EEPROM_CS
    TRISBbits.TRISB5 = 0;   // MU150_CS
    TRISFbits.TRISF0 = 0;   // MU150_CS2
    RPC7Rbits.RPC7R = 0b00011;  // SDO1     -->     see TABLE 13-2 OUTPUT PIN SELECTION
    SDI1Rbits.SDI1R = 0b1001;   // SDI1     -->     see TABLE 13-1 INPUT PIN SELECTION
    
    // bit : SPI data length, 8 or 16 or 32
    if(data_bit == 8){
        bit_mode[0] = 0;
        bit_mode[1] = 0;
    }else if(data_bit == 16){
        bit_mode[0] = 1;
        bit_mode[1] = 0;
    }else if(data_bit == 32){
        bit_mode[0] = 0;
        bit_mode[1] = 1;
    }else{
        bit_mode[0] = 1;
        bit_mode[1] = 0;
    }
    
    // mode : SPI operation mode
    // 0 = idle low & rising edge
    // 1 = idle low & falling edge
    // 2 = idle high & falling edge
    // 3 = idle high & rising edge
    if(mode == 0){
        mode_select[0] = 0;
        mode_select[1] = 0;
    }else if(mode == 1){
        mode_select[0] = 0;
        mode_select[1] = 1;
    }else if(mode == 2){
        mode_select[0] = 1;
        mode_select[1] = 0;
    }else if(mode == 3){
        mode_select[0] = 1;
        mode_select[1] = 1;
    }
    
    // Register setting
    //SPI1BRG = 299;                  // Fsck = Fpb/(2*(SPIxBRG+1)), 100k = 60M/(2*300)
    SPI1BRG = 49;                  // Fsck = Fpb/(2*(SPIxBRG+1)), 600k = 60M/(2*50)
    SPI1STATCLR=0x40;               // clear the Overflow
    SPI1CON = 0x0000;               // clear the register
    dummy = SPI1BUF;                // clear the buffer by using dummy
    SPI1CONbits.MSTEN = 1;          // 1 = master mode / 0 = slave mode
    SPI1CONbits.MODE16 = bit_mode[0];
    SPI1CONbits.MODE32 = bit_mode[1];
    SPI1CONbits.SMP = 1;            // 1 = input data sampled at "end" of data output time/ 0 = middle
    SPI1CONbits.CKP = mode_select[0];       // 1: high idle      0: low idle
    SPI1CONbits.CKE = mode_select[1];       // 1: active->idle   0: idle->active
    SPI1CONbits.ON = 1;             // SPI Start
}

unsigned int _SPI_WRITE(unsigned int data){
	unsigned int ret;
	
 	ret = SPI1BUF;                 // dummy read of the SPI1BUF register to clear the SPIRBF flag
	SPI1BUF = data;                 // write the data out to the SPI peripheral
	
	while(!SPI1STATbits.SPIRBF);	// wait for the data to be sent out
	
	ret = SPI1BUF;	
	return ret;
}



static unsigned int _EEPROM_ReadStatus(){
	unsigned int ret;
	EEPROM_SELECT
	ret = _SPI_WRITE(READ_STAT_REG);
	ret = _SPI_WRITE(0x00);
	EEPROM_DESELECT
	return ret;
}

void EEPROM_WRITE(unsigned int ch, unsigned int type, unsigned int data){
	unsigned int temp, addr;
    unsigned int a1, a2, b1, b2, b3, b4;
    addr = ((ch*EE_TYPE_MAX + type)<<2) & 0x3FFF;
    	
    a1 = (addr>>8)&0xFF;
	a2 = addr&0xFF;
	b1 = (data>>24)&0xFF;
    b2 = (data>>16)&0xFF;
    b3 = (data>>8)&0xFF;
	b4 = data&0xFF;
   
    do{
  		EEPROM_SELECT
		_SPI_WRITE(WRITE_ENABLE);
		EEPROM_DESELECT
		temp = _EEPROM_ReadStatus();
		temp = temp & (WIP | WEL);
	}while(temp != WEL);//temp != WEL
 
	EEPROM_SELECT
	_SPI_WRITE(WRITE_MEMORY);
	temp = _SPI_WRITE(a1);
	temp = _SPI_WRITE(a2);
	temp = _SPI_WRITE(b1);
	temp = _SPI_WRITE(b2);
    temp = _SPI_WRITE(b3);
    temp = _SPI_WRITE(b4);
	EEPROM_DESELECT
}

unsigned int EEPROM_READ(unsigned int ch, unsigned int type){
	unsigned int temp, addr;
    unsigned int a1, a2, b1, b2, b3, b4;

    addr = ((ch*EE_TYPE_MAX + type)<<2) & 0x3FFF;

	a1 = (addr>>8)&0xFF;
	a2 = addr&0xFF;

	EEPROM_SELECT
	temp = _SPI_WRITE(READ_MEMORY);// Read Commend 
	temp = _SPI_WRITE(a1);		// MSB address
	temp = _SPI_WRITE(a2);		// LSB address
	b1 = _SPI_WRITE(0x00);		// Dummy data for clock
	b2 = _SPI_WRITE(0x00);		// Dummy data for clock
    b3 = _SPI_WRITE(0x00);		// Dummy data for clock
    b4 = _SPI_WRITE(0x00);		// Dummy data for clock
	EEPROM_DESELECT
	return (b1<<24) | (b2<<16) | (b3<<8) | b4;
}




//
//void write_romL(unsigned int addr1,unsigned int addr2,  unsigned long data)
//{
//unsigned int L1,L2;
//
////	addr = addr1*MAX_DATA + addr2; 
//
//	L1 =(unsigned int)((data>>16)&0xFFFF);
//	L2 =(unsigned int)(data & 0x0000FFFF);
//	write_rom(addr1,addr2,L2);
//	write_rom(addr1,addr2+1,L1);
//}
//void write_romf(unsigned int addr1,unsigned int addr2,  float val)
//{
//	unsigned int c1,c2;
//
////	addr = addr1*MAX_DATA + addr2; 
//
//	fl2l.f=val;
//	c1=(unsigned int)(fl2l.l>>16);
//	c2=(unsigned int)(fl2l.l);
//	write_rom(addr1,addr2,c2);
//	write_rom(addr1,addr2+1,c1);
//
//}
//float read_romf(unsigned int addr1,unsigned int addr2)
//{
//unsigned long c1,c2;
//
//	c1=((unsigned long)read_rom(addr1, addr2)) << 16;
//	c2=((unsigned long)read_rom(addr1, addr2+1))&0x0000FFFF;
//	fl2l.l=c2 + c1;
//	return fl2l.f;
//}
//
//long read_romL(unsigned int addr1,unsigned int addr2)
//{
//long LL;
//
//	LL=(long)read_rom(addr1, addr2+1);
//	LL<<=16;
//	LL+=read_rom(addr1, addr2);
//	return LL;
//}



unsigned int AS5047_READ(){
	unsigned int temp, addr;
    unsigned int a1, a2, b1, b2;

	addr = 0xFFFF;      // Measured angle without dynamic angle error compensation
	a1 = (addr>>8)&0xFF;
	a2 = addr     &0xFF;

    AS5047_SELECT	
	b1 = _SPI_WRITE(a1);		// Dummy data for clock
	b2 = _SPI_WRITE(a2);		// Dummy data for clock
	AS5047_DESELECT
            
//    AS5047_SELECT	
//	b1 = _SPI_WRITE(0x00);		// Dummy data for clock
//	b2 = _SPI_WRITE(0x00);		// Dummy data for clock
//	AS5047_DESELECT
   	temp =  (b1<<8) | b2;

	return temp&0x03FFF;
}



#define MU150_OP_ACTIVATE       0xB0
#define MU150_OP_SENSOR_DATA    0xA6
#define MU150_OP_STATUS         0xF5
#define MU150_OP_READ_REG       0x97
#define MU150_OP_WRITE_REG      0xD2
#define MU150_OP_REG_STATUS     0xAD

short MULTI_TURN_DATA = 0;


unsigned int MU150_ReadSensor(){
    unsigned int temp;
    unsigned int b1, b2, b3;
    
    MU150_SELECT
	temp = _SPI_WRITE(MU150_OP_SENSOR_DATA);
	b1 = _SPI_WRITE(0x00);		// Dummy data for clock
	b2 = _SPI_WRITE(0x00);		// Dummy data for clock
    b3 = _SPI_WRITE(0x00);		// Dummy data for clock
	MU150_DESELECT
    
    return (((b1<<16) | (b2<<8) | b3) >> 7);
}
unsigned int MU150_ReadSensorRaw(){
    unsigned int temp;
    unsigned int b1, b2, b3, b4;
    
    MU150_SELECT
	temp = _SPI_WRITE(MU150_OP_SENSOR_DATA);
	b1 = _SPI_WRITE(0x00);		// Dummy data for clock
	b2 = _SPI_WRITE(0x00);		// Dummy data for clock
    b3 = _SPI_WRITE(0x00);		// Dummy data for clock
    b4 = _SPI_WRITE(0x00);		// Dummy data for clock
	MU150_DESELECT
    return ((b1<<24) | (b2<<16) | (b3<<8) | b4);
}

unsigned int MU150_ReadSensor_MultiTurn(){
    unsigned int temp;
    unsigned int b1, b2, b3, b4, b5;
    
    MU150_SELECT
	temp = _SPI_WRITE(MU150_OP_SENSOR_DATA);
	b1 = _SPI_WRITE(0x00);		// Dummy data for clock
	b2 = _SPI_WRITE(0x00);		// Dummy data for clock
    b3 = _SPI_WRITE(0x00);		// Dummy data for clock
    b4 = _SPI_WRITE(0x00);
    b5 = _SPI_WRITE(0x00);
	MU150_DESELECT
            
    MULTI_TURN_DATA = (short)((b1<<8) | (b2));
    return (((b3<<16) | (b4<<8) | b5) >> 7);
}

unsigned int MU150_ReadRegister(unsigned int addr){
    unsigned int temp;
    unsigned int b1, b2, b3;
    int i;
        
    MU150_SELECT
	temp = _SPI_WRITE(MU150_OP_READ_REG);
	temp = _SPI_WRITE(addr);
    MU150_DESELECT
    
    
    do{
        for(i=0; i<30; i++)
            ;
        MU150_SELECT
        temp = _SPI_WRITE(MU150_OP_REG_STATUS);            
        b1 = _SPI_WRITE(0x00);		// Dummy data for clock
        b2 = _SPI_WRITE(0x00);		// Dummy data for clock
    //    b3 = _SPI_WRITE(0x00);		// Dummy data for clock
        MU150_DESELECT
                
    }while((b1&0x02) == 0x02);
            
    return b1 | (b2<<8);// | (b3<<16);
}
unsigned int MU150_ReadRegister2(unsigned int addr){
    unsigned int temp;
    unsigned int b1, b2, b3;
    int i;
        
    MU150_SELECT2
	temp = _SPI_WRITE(MU150_OP_READ_REG);
	temp = _SPI_WRITE(addr);
    MU150_DESELECT2
    
    
    do{
        for(i=0; i<30; i++)
            ;
        MU150_SELECT2
        temp = _SPI_WRITE(MU150_OP_REG_STATUS);            
        b1 = _SPI_WRITE(0x00);		// Dummy data for clock
        b2 = _SPI_WRITE(0x00);		// Dummy data for clock
    //    b3 = _SPI_WRITE(0x00);		// Dummy data for clock
        MU150_DESELECT2
                
    }while((b1&0x02) == 0x02);
            
    return b1 | (b2<<8);// | (b3<<16);
}


unsigned int MU150_WriteRegister(unsigned int addr, unsigned int data){
    unsigned int temp;
    
    MU150_SELECT
    temp = _SPI_WRITE(MU150_OP_WRITE_REG);
    temp = _SPI_WRITE(addr);
    temp = _SPI_WRITE(data);
    MU150_DESELECT
    return 0;
}
unsigned int MU150_WriteRegister2(unsigned int addr, unsigned int data){
    unsigned int temp;
    
    MU150_SELECT2
    temp = _SPI_WRITE(MU150_OP_WRITE_REG);
    temp = _SPI_WRITE(addr);
    temp = _SPI_WRITE(data);
    MU150_DESELECT2
    return 0;
}



int Push_EEPROM_Ring(unsigned int ch, unsigned int type, unsigned int data){
	unsigned int new_head = _eeprom_ring.head = (_eeprom_ring.head+1)%EEPROM_BUFF_SIZE;

	if(new_head == _eeprom_ring.tail){
		if(new_head == 0)
			_eeprom_ring.head = EEPROM_BUFF_SIZE-1;
		else
			_eeprom_ring.head -= 1;
		return -1;
	}
	_eeprom_ring.ch[new_head] = ch;
	_eeprom_ring.type[new_head] = type;
    _eeprom_ring.data[new_head] = data;
		
	return 0;
}

int Handler_EEPROM_Ring(){
	unsigned int new_tail;
	if(_eeprom_ring.head != _eeprom_ring.tail){
		new_tail = (_eeprom_ring.tail+1)%EEPROM_BUFF_SIZE;

        EEPROM_WRITE(_eeprom_ring.ch[new_tail], _eeprom_ring.type[new_tail], _eeprom_ring.data[new_tail]);
        _eeprom_ring.tail = new_tail;
		return 1;
	}
	return 0;
}

static void _Init_EEPROM_Ring(){
	memset(&_eeprom_ring, 0, sizeof(EEPROM_RING));
}





int Push_MU_Ring(unsigned int addr, unsigned int data){
    unsigned int new_head = (_mu_ring.head+1)%MU_BUFF_SIZE;

	if(new_head == _mu_ring.tail){
		return -1;
	}
	_mu_ring.addr[new_head] = addr;
    _mu_ring.data[new_head] = data;
    _mu_ring.head = new_head;    
	return 0;
}

int Handler_MU_Ring(){
	unsigned int new_tail;
	if(_mu_ring.head != _mu_ring.tail){
		new_tail = (_mu_ring.tail+1)%MU_BUFF_SIZE;

        MU150_WriteRegister(_mu_ring.addr[new_tail], _mu_ring.data[new_tail]);
        _mu_ring.tail = new_tail;
		return 1;
	}
	return 0;
}

void Init_MU_Ring(){
	memset(&_mu_ring, 0, sizeof(MU_RING));
}



void InitPVL(){
#ifdef BOARD_TYPE_25
    ANSELBbits.ANSB3 = 0;
    TRISBbits.TRISB3 = 0;
    PRESET_OFF
#else
    // Preset of PVL
    ANSELBbits.ANSB3 = 0;
    TRISBbits.TRISB3 = 0;
    PRESET_OFF
            
    // NWRN
    ANSELCbits.ANSC2 = 0;
    TRISCbits.TRISC2 = 1;
#endif
}


int bank_0_index = 0;
int bank_1_index = 0;
int bank_1_cnt = 0;

void MU_EEPROM_SETTING_BANK_0(int index){
    switch(index){
        case 0:
            MU150_WriteRegister(0x0D, 128);
            break;
        case 1:
            //MU150_WriteRegister(0x0E, 70);    // filter 6
//            MU150_WriteRegister(0x0E, 68);      // filter 4
            MU150_WriteRegister(0x0E, 4);      // filter 4
            break;
        case 2:
            MU150_WriteRegister(0x0F, 5);
            break;
        case 3:
//            MU150_WriteRegister(0x10, 46);
            MU150_WriteRegister(0x10, 0);
            break;
        case 4:
//            MU150_WriteRegister(0x11, 181);
            MU150_WriteRegister(0x11, 165);
            break;
        case 5:
            MU150_WriteRegister(0x12, 0);
            break;
    }
}
void MU_EEPROM_SETTING_BANK_0_2(int index){
    switch(index){
        case 0:
            MU150_WriteRegister2(0x0D, 128);
            break;
        case 1:
            //MU150_WriteRegister(0x0E, 70);    // filter 6
//            MU150_WriteRegister(0x0E, 68);      // filter 4
            MU150_WriteRegister2(0x0E, 4);      // filter 4
            break;
        case 2:
            MU150_WriteRegister2(0x0F, 5);
            break;
        case 3:
//            MU150_WriteRegister(0x10, 46);
            MU150_WriteRegister2(0x10, 0);
            break;
        case 4:
//            MU150_WriteRegister(0x11, 181);
            MU150_WriteRegister2(0x11, 165);
            break;
        case 5:
            MU150_WriteRegister2(0x12, 0);
            break;
    }
}

void MU_EEPROM_SETTING_BANK_1(int index){
    switch(index){
        case 0:
            MU150_WriteRegister(0x00, 32);
            break;
        case 1:
            MU150_WriteRegister(0x01, 39);
            break;
        case 2:
            MU150_WriteRegister(0x02, 31);
            break;
        case 3:
            //MU150_WriteRegister(0x03, 24);    // low
            //MU150_WriteRegister(0x03, 16);      // high
            MU150_WriteRegister(0x03, 0);       // max
            break;
        case 4:
            MU150_WriteRegister(0x04, 0);
            break;
        case 5:
            MU150_WriteRegister(0x05, 48);
            break;
        case 6:
            //MU150_WriteRegister(0x06, 163);   // low
            //MU150_WriteRegister(0x06, 211);     // high
            MU150_WriteRegister(0x06, 51);      // max
            break;
        case 7:
            MU150_WriteRegister(0x07, 0);
            break;
        case 8:
            MU150_WriteRegister(0x08, 0);
            break;
        case 9:
            MU150_WriteRegister(0x09, 0);
            break;
        case 10:
            MU150_WriteRegister(0x0A, 0);
            break;
        case 11:
            MU150_WriteRegister(0x0B, 0);
            break;
        case 12:
            MU150_WriteRegister(0x0C, 255);
            break;
    }
}

int MU_EEPROM_CHECK_BANK_0(){
    unsigned int temp;
    temp = (MU150_ReadRegister(0x0D)>>8)&0xFF;
    
#ifdef seokgyun_debug
    unsigned char canTxData[8] = {0,};
    canTxData[0] = (temp >> 0) & 0xFF;
    canTxData[1] = (temp >> 8) & 0xFF;
    canTxData[2] = (temp >> 16) & 0xFF;
    canTxData[3] = (temp >> 24) & 0xFF;
    Push_CAN_TX_Ring(0x777, 4, canTxData);     
#endif
    if(temp != 128)
    {
        return 0;
    }
    //if(temp != 70){   // filter 6
//    if(temp != 68){     // filter 4
    
    temp = (MU150_ReadRegister(0x0E)>>8)&0xFF;
#ifdef seokgyun_debug
    unsigned char canTxData2[8] = {0,};
    canTxData2[0] = (temp >> 0) & 0xFF;
    canTxData2[1] = (temp >> 8) & 0xFF;
    canTxData2[2] = (temp >> 16) & 0xFF;
    canTxData2[3] = (temp >> 24) & 0xFF;
    Push_CAN_TX_Ring(0x778, 4, canTxData2); 
#endif
    if(temp != 4){     // filter 4
        return 0;
    }
    
    
    
    temp = (MU150_ReadRegister(0x0F)>>8)&0xFF;
#ifdef seokgyun_debug
    unsigned char canTxData3[8] = {0,};
    canTxData3[0] = (temp >> 0) & 0xFF;
    canTxData3[1] = (temp >> 8) & 0xFF;
    canTxData3[2] = (temp >> 16) & 0xFF;
    canTxData3[3] = (temp >> 24) & 0xFF;
    Push_CAN_TX_Ring(0x779, 4, canTxData3); 
#endif
    if(temp != 5){
        return 0;
    }
    
    

    temp = (MU150_ReadRegister(0x10)>>8)&0xFF;
#ifdef seokgyun_debug
    unsigned char canTxData4[8] = {0,};
    canTxData4[0] = (temp >> 0) & 0xFF;
    canTxData4[1] = (temp >> 8) & 0xFF;
    canTxData4[2] = (temp >> 16) & 0xFF;
    canTxData4[3] = (temp >> 24) & 0xFF;
    Push_CAN_TX_Ring(0x77A, 4, canTxData4); 
#endif
//    if(temp != 46){
    if(temp != 0){
        return 0;
    }
    

    temp = (MU150_ReadRegister(0x11)>>8)&0xFF;
//    if(temp != 181){
#ifdef seokgyun_debug
    unsigned char canTxData5[8] = {0,};
    canTxData5[0] = (temp >> 0) & 0xFF;
    canTxData5[1] = (temp >> 8) & 0xFF;
    canTxData5[2] = (temp >> 16) & 0xFF;
    canTxData5[3] = (temp >> 24) & 0xFF;
    Push_CAN_TX_Ring(0x77B, 4, canTxData5); 
#endif
    if(temp != 165){
        return 0;
    }
    temp = (MU150_ReadRegister(0x12)>>8)&0xFF;
    
#ifdef seokgyun_debug
    unsigned char canTxData6[8] = {0,};
    canTxData6[0] = (temp >> 0) & 0xFF;
    canTxData6[1] = (temp >> 8) & 0xFF;
    canTxData6[2] = (temp >> 16) & 0xFF;
    canTxData6[3] = (temp >> 24) & 0xFF;
    Push_CAN_TX_Ring(0x77C, 4, canTxData6); 
#endif

    if(temp != 0){
        return 0;
    }
    return 1;
}

int MU_EEPROM_CHECK_BANK_0_2(){
    unsigned int temp;
    temp = (MU150_ReadRegister2(0x0D)>>8)&0xFF;
    if(temp != 128){
        return 0;
    }
    temp = (MU150_ReadRegister2(0x0E)>>8)&0xFF;
    //if(temp != 70){   // filter 6
//    if(temp != 68){     // filter 4
    if(temp != 4){     // filter 4
        return 0;
    }
    temp = (MU150_ReadRegister2(0x0F)>>8)&0xFF;
    if(temp != 5){
        return 0;
    }
    temp = (MU150_ReadRegister2(0x10)>>8)&0xFF;
//    if(temp != 46){
    if(temp != 0){
        return 0;
    }
    temp = (MU150_ReadRegister2(0x11)>>8)&0xFF;
//    if(temp != 181){
    if(temp != 165){
        return 0;
    }
    temp = (MU150_ReadRegister2(0x12)>>8)&0xFF;
    if(temp != 0){
        return 0;
    }
    return 1;
}


int MU_EEPROM_CHECK_BANK_1(){
    unsigned int temp;
    temp = (MU150_ReadRegister(0x00)>>8)&0xFF;
    if(temp != 32){
        return 0;
    }
    temp = (MU150_ReadRegister(0x01)>>8)&0xFF;
    if(temp != 39){
        return 0;
    }
    temp = (MU150_ReadRegister(0x02)>>8)&0xFF;
    if(temp != 31){
        return 0;
    }
    temp = (MU150_ReadRegister(0x03)>>8)&0xFF;
    //if(temp != 24){   // low
    //if(temp != 16){     // high
    if(temp != 0){     // max
        return 0;
    }
    temp = (MU150_ReadRegister(0x04)>>8)&0xFF;
    if(temp != 0){
        return 0;
    }
    temp = (MU150_ReadRegister(0x05)>>8)&0xFF;
    if(temp != 48){
        return 0;
    }
    temp = (MU150_ReadRegister(0x06)>>8)&0xFF;
    //if(temp != 163){  // low
    //if(temp != 211){    // high
    if(temp != 51){    // max
        return 0;
    }
    temp = (MU150_ReadRegister(0x07)>>8)&0xFF;
    if(temp != 0){
        return 0;
    }
    temp = (MU150_ReadRegister(0x08)>>8)&0xFF;
    if(temp != 0){
        return 0;
    }
    temp = (MU150_ReadRegister(0x09)>>8)&0xFF;
    if(temp != 0){
        return 0;
    }
    temp = (MU150_ReadRegister(0x0A)>>8)&0xFF;
    if(temp != 0){
        return 0;
    }
    temp = (MU150_ReadRegister(0x0B)>>8)&0xFF;
    if(temp != 0){
        return 0;
    }
    temp = (MU150_ReadRegister(0x0C)>>8)&0xFF;
    if(temp != 255){
        return 0;
    }
    return 1;
}



