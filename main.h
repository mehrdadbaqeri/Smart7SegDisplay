#include <avr\io.h>
#include <util\twi.h>
#include <stdio.h>
#include <avr\interrupt.h>
#include <avr\pgmspace.h>
#include <avr\sleep.h> 
#include <util\delay.h>
#include <avr/eeprom.h>
#include <string.h>


#define F_CPU 3686400   // Clock Speed
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1

#define KEY	0X04
#define FREQ 4

/*===================EEPROM Addresses==========================
			rightshiftFlag---------->0x0002
			columnScrollFlag-------->0x0004
			scrollSpeed------------->0x0006
			sentenceSize------------>0x000A
			sentence[140]----------->0x0010 to 0x009E
			userCharDefine[75]---------->0x00A0 to 0x00EA
==============================================================*/
//=======================================Hardware dependent functions, constants and macros
void initHardware(void);//Initializing all parameters of each hardware component   
unsigned char USART_Receive( void );// Receiving the text from USART
void USART_Transmit( unsigned char data );// Sending by USART
void read_adc(void);
void adc_start_conversion(void);
void loadingPreviousState(void);
//=======================================Hardware independent functions, constants and macros 
int main(void);
void userDefinesave(char inputText[]);
char char2hex(char c);
void knobShowLetter(void);
void messageGettingUSART(char inputText[],char index);
void getCommand(char inputText[],char index);
void initialPosition(void);
//=======================================Tasks
void ShowLetterTask(void);// This task is responsible for showing ond letter
void ScrollLetterTask(void);// This task is responsible for scrolling letter by letter
void ScrollColumnTask(void);// This task is responsible for scrolling column by column
//===================================================================

//======================================= Global variable definitions
static uint8_t sentenceSize=0;//The entered text size
unsigned char inputSize=0;
static char columnScrollFlag=1;
static char rightScrollingFlag=0;
char sentenceCompeleteFlag=0;
static volatile char devicePosition=1;
static int scrollSpeed=10;
unsigned char maxModules=1;


int16_t showColumnIndex=0;//This variable keeps the current column index in showLetterTask
volatile int16_t columnIndex;// This variable keeps the current column index in columns array
char knobFlag=0;
char knobIndex=0;
char knobColumnIndex=33;
char knobStartFlag=0;
int sentenceLetterIndex;
char knobChangeFlag=0;
char offFlag=0;
char usartReceiveFlag=0;

//===================================================================

//======================================= Timer variable definitions
static unsigned char showLetterTaskTime=0;// This is a time counter for showing a letter  
volatile int scrollColumnTaskTime=0;// This is a time counter for scrolling the columns  
 //===================================================================

//======================================= Arrays definitions
volatile char sentence[140]={};//78,69,77,79,123,65,78,68,123,77,69,72,82,68,65,68,123,65,78,68,123,78,65,78,68,73,78,73};// This array the ASCI code each letter is received from PC
char colArray[5]={0x10,0x08,0x04,0x02,0x01};
const prog_uint8_t rowArray[640] PROGMEM={
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.0
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.1
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.2
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.3
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.4
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.5
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.6
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.7
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.8
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.9
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.10
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.11
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.12
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.13
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.14
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.15
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.16
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.17
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.18
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.19
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.20
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.21
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.22
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.23
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.24
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.25
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.26
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.27
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.28
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.29
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.30
                 0x00,0xFE,0x82,0xFE,0x00,//Letter  No.31
                 0x00,0x00,0x00,0x00,0x00,//Space  No.32
				//=========================================Space for user defined characters
                 0x00,0x00,0x3D,0x00,0x00,//Letter ! No.33
                 0x00,0xE0,0x00,0xE0,0x00,//Letter " No.34
                 0x00,0x00,0x00,0x00,0x00,//Letter # it is not defined No.35
                 0x00,0x00,0x00,0x00,0x00,//Letter $ it is not defined No.36
                 0x00,0x00,0x00,0x00,0x00,//Letter % it is not defined No.37
                 0x00,0x00,0x00,0x00,0x00,//Letter & it is not defined No.38
                 0x00,0x00,0x00,0x00,0x00,//Letter ' it is not defined No.39
                 0x00,0x3C,0x42,0x81,0x00,//Letter ( No.40
                 0x00,0x81,0x42,0x3C,0x00,//Letter ) No.41
                 0x00,0x00,0x00,0x00,0x00,//Letter * it is not defined No.42

                 0x00,0x00,0x00,0x00,0x00,//Letter + it is not defined No.43
                 0x00,0x05,0x06,0x00,0x00,//Letter , No.44 
                 0x00,0x08,0x08,0x08,0x00,//Letter - No.45 
                 0x00,0x06,0x06,0x00,0x00,//Letter . No.46 
                 0x02,0x04,0x08,0x10,0x20,//Letter / No.47 
                 //Start of the numbers
                 0x7C,0x8A,0x92,0xA2,0x7C,//Letter 0 No.48 
                 0x00,0x42,0xFE,0x02,0x00,//Letter 1 No.49
                 0x42,0x86,0x8A,0x92,0x62,//Letter 2 No.50
                 0x84,0x82,0xA2,0xD2,0x8C,//Letter 3 No.51
                 0x18,0x28,0x48,0xFE,0x08,//Letter 4 No.52
                 0xE4,0xA2,0xA2,0xA2,0x9C,//Letter 5 No.53 
                 0x3C,0x52,0x92,0x92,0x0C,//Letter 6 No.54
                 0x80,0x8E,0x90,0xA0,0xC0,//Letter 7 No.55
                 0x6C,0x92,0x92,0x92,0x6C,//Letter 8 No.56
                 0x60,0x92,0x92,0x94,0x78,//Letter 9 No.57

                 0x00,0x36,0x36,0x00,0x00,//Letter : No.58
                 0x00,0x35,0x36,0x00,0x00,//Letter ; No.59 
                 0x00,0x00,0x00,0x00,0x00,//Letter < it is not defined No.60
                 0x00,0x00,0x00,0x00,0x00,//Letter = it is not defined No.61
                 0x00,0x00,0x00,0x00,0x00,//Letter > it is not defined No.62
                 0x20,0x40,0x45,0x48,0x30,//Letter ? No.63
                 0x00,0x00,0x00,0x00,0x00,//Letter @ it is not defined No.64
                 //Capital letters
                 0x7E,0x88,0x88,0x88,0x7E,//Letter A No.65
                 0xFE,0x92,0x92,0x92,0x6C,//Letter B No.66
                 0x7C,0x82,0x82,0x82,0x44,//Letter C No.67
                 0xFE,0x82,0x82,0x44,0x38,//Letter D No.68
                 0xFE,0x92,0x92,0x92,0x82,//Letter E No.69
                 0xFE,0x90,0x90,0x90,0x80,//Letter F No.70
                 0x7C,0x82,0x92,0x92,0x5C,//Letter G No.71
                 0xFE,0x10,0x10,0x10,0xFE,//Letter H No.72
                 0x00,0x82,0xFE,0x82,0x00,//Letter I No.73
                 0x0C,0x02,0x82,0xFC,0x80,//Letter J No.74

                 0xFE,0x10,0x28,0x44,0x82,//Letter K No.75
                 0xFE,0x02,0x02,0x02,0x02,//Letter L No.76
                 0xFE,0x40,0x30,0x40,0xFE,//Letter M No.77
                 0xFE,0x20,0x10,0x08,0xFE,//Letter N No.78
                 0x7C,0x82,0x82,0x82,0x7C,//Letter O No.79
                 0xFE,0x90,0x90,0x90,0x60,//Letter P No.80
                 0x7C,0x82,0x86,0x82,0x7D,//Letter Q No.81
                 0xFE,0x90,0x98,0x94,0x62,//Letter R No.82
                 0x64,0x92,0x92,0x92,0x4C,//Letter S No.83
                 0x80,0x80,0xFE,0x80,0x80,//Letter T No.84
                 
                 0xFC,0x02,0x02,0x02,0xFC,//Letter U No.85
                 0xF8,0x04,0x02,0x04,0xF8,//Letter V No.86
                 0xFC,0x02,0x1C,0x02,0xFC,//Letter W No.87
                 0xC6,0x28,0x10,0x28,0xC6,//Letter X No.88
                 0xE0,0x10,0x0E,0x10,0xE0,//Letter Y No.89
                 0x86,0x8A,0x92,0xA2,0xC2,//Letter Z No.90
                 //End of Capital letters                 
                 
                 0x00,0xFF,0x81,0x81,0x00,//Letter [ No.91
                 0x00,0x00,0x00,0x00,0x00,//Letter < it is not defined No.92
                 0x00,0x81,0x81,0xFF,0x00,//Letter ] No.93                  
                 0x00,0x00,0x00,0x00,0x00,//Letter ^ it is not defined No.94
                 0x00,0x00,0x00,0x00,0x00,//Letter _ it is not defined No.95
                 0x00,0x00,0x00,0x00,0x00,//Letter ` it is not defined No.96
                 
                 //Small letters
                 0x04,0x2A,0x2A,0x2A,0x1E,//Letter a No.97                            
                 0xFE,0x22,0x22,0x22,0x1C,//Letter b No.98
                 0x1C,0x22,0x22,0x22,0x12,//Letter c No.99
                 0x1C,0x22,0x22,0x22,0xFE,//Letter d No.100
                 0x1C,0x2A,0x2A,0x2A,0x1A,//Letter e No.101
                 0x20,0x7F,0xA0,0x80,0x00,//Letter f No.102
                 0x19,0x25,0x25,0x25,0x1E,//Letter g No.103
                 0xFE,0x20,0x20,0x20,0x1E,//Letter h No.104
                 0x00,0x22,0xBE,0x02,0x00,//Letter i No.105
                 0x02,0x01,0x21,0xBE,0x00,//Letter j No.106

                 0x00,0xFE,0x08,0x14,0x22,//Letter k No.107
                 0x00,0x82,0xFE,0x02,0x00,//Letter l No.108
                 0x3E,0x20,0x3E,0x20,0x1E,//Letter m No.109
                 0x3E,0x20,0x20,0x20,0x1E,//Letter n No.110
                 0x1C,0x22,0x22,0x22,0x1C,//Letter o No.111
                 0x3F,0x24,0x24,0x24,0x18,//Letter p No.112
                 0x18,0x24,0x24,0x24,0x3F,//Letter q No.113
                 0x20,0x1E,0x20,0x20,0x10,//Letter r No.114
                 0x12,0x2A,0x2A,0x2A,0x24,//Letter s No.115
                 0x20,0x7C,0x22,0x22,0x00,//Letter t No.116
                 
                 0x3C,0x02,0x02,0x02,0x3C,//Letter u No.117
                 0x38,0x04,0x02,0x04,0x38,//Letter v No.118
                 0x3C,0x02,0x1C,0x02,0x3C,//Letter w No.119
                 0x22,0x14,0x08,0x14,0x22,//Letter x No.120
                 0x38,0x05,0x05,0x05,0x3E,//Letter y No.121
                 0x22,0x26,0x2A,0x32,0x22,//Letter Z No.122
                 //End of Small letters
                 0x0E,0x04,0x08,0x10,0x20, //Tick No.123
                 0x00,0xFE,0x82,0xFE,0x00, //Letter No.124
                 0x00,0xFE,0x82,0xFE,0x00, //Letter No.125
                 0x00,0xFE,0x82,0xFE,0x00, //Letter No.126
                 0x00,0xFE,0x82,0xFE,0x00 //Letter No.127               
                };

//===================================================================