#include "main.h"
#include "Usart.h"
#include "i2c.h"
static volatile unsigned char configAddressFlag = 1;
//volatile unsigned char  LWPinDelayCounter = 0;
volatile unsigned char configTimeoutCounter;
extern unsigned char volatile i2cSendMessageBuf[];
extern unsigned char volatile SIZE;
extern volatile unsigned char indexRec;
volatile char syncStartFlag=0;
volatile char configStatingFlag=0;
static unsigned int knobCounter=0;
volatile char knobSend=0;
static char knobSentence[140];
static unsigned char knobSize=2;
static char userDefine[80];


// ====================================== Hardware dependent functions
void initHardware(void)
{
//--------------------------I2C initializing
	InitI2C();
	
//--------------------------Timer1 initializing for 1.25 msec
	TCCR1A=0x00;
  	TCCR1B=0x0A;
  	TCNT1=0;
  	OCR1A=576; 
 	TIMSK=0x10;

//--------------------------Ports initializing
	
	//PORTA Pin0 to 4 for controlling the columns- Pin7 for reading the ADC 
    PORTA=0x00;
    DDRA=0x1F;  
	
	// Pin0 to 7 for controlling the rows
    PORTB=0x00; 
    DDRB=0xFF;  


	//PORTC: Pin0 and 1 for I2C- Pin3 as left pin- Pin5 for sending characters by knob through I2C(SW7))
	//PORTC: Pin7 for selecting the characters provided by knob
	DDRC =0x00;
	PORTC |=0X0F;

	//PORTD: Pin0 and 1 for USART- Pin3 as right pin
	DDRD = 0b0001011;
	PORTD = 0x00;

//-----------------------USART init
    USART_Init(MYUBRR);      
	
//-----------------------ADC init
	ADMUX|=(0<<REFS1)|(1<<REFS0);
	ADCSRA|=(1<<ADEN)|(1<<ADIE)|0x86;
	ADMUX=(ADMUX&0xF0)|0x07;
	ADCSRA |= (1<<ADSC)|(1<<ADIE);
	set_sleep_mode(SLEEP_MODE_ADC);
}  


void adc_start_conversion()
{
	//set ADC channel
	ADMUX=(ADMUX&0xF0)|0x07;
	//Start conversion with Interrupt after conversion
	//enable global interrupts
	ADCSRA |= (1<<ADSC)|(1<<ADIE);
}
ISR(ADC_vect)
{
	
	int16_t ADData;
	
	static int16_t preAdc=0,adc=0;
	static char prevButton=0;
	char currButton;
	ADData = ADCL;
	ADData += (ADCH<<8);
	adc=ADData;
	if(!(PINC&0b10000000))
	{
		currButton=bit_is_clear(PINC,6);//Checking the knob select character button
		if (currButton && !prevButton)
		{
			knobSentence[0]='0';
			knobSentence[1]='8';
			knobSentence[knobSize++]=knobColumnIndex;
			knobChangeFlag=1;
			knobColumnIndex=123;// Showing correct sign after selecting
		}
		prevButton=currButton;
		knobFlag=1;
		if(adc>870)//Forwarding through the letters
		{
			knobColumnIndex++;
			if(knobColumnIndex>124)
				knobColumnIndex=124;
			preAdc=adc;
		}
		else if(adc<100)//Backwarding through the letters
		{
			knobColumnIndex--;
			if(knobColumnIndex<33)
				knobColumnIndex=33;
			preAdc=adc;
		}
	}
}

ISR(USART_RXC_vect )
{
	unsigned char Rev;
	int i;
	static unsigned char index=0;
	static char inputText[150]={0};
	Rev=UDR;
	sei();
	if(Rev==38 || Rev=='&')//Checking the end character '&'
	{
		//Preparing buffer and size value for sending the message
		SIZE = index;
		strcpy(i2cSendMessageBuf,inputText);
		syncStartFlag=0;
		TWCR = (1<<TWINT)|(1<<TWEN)| _BV(TWSTA)|(1<<TWEA)|(1<<TWIE);
		getCommand(inputText,index);
		index=0;
	}
	else
	{
		if(inputText[index-1]=='^')
		{
			index-=1;
			inputText[index]=(char2hex(Rev));
			Usart_PutChar(inputText[index-1]);
			index++;
		}
		else 
		{
			inputText[index++]=Rev;
		}
	}
}
ISR(TIMER1_COMPA_vect)
{
	static char prevSendButton=0;
	char currSendButton;
	if(configStatingFlag)
		configTimeoutCounter++;
	knobCounter++;
	if(!(PINC&0b10000000))
	{
		if(knobCounter>350)
		{
			
			knobCounter=0;
			knobFlag=1;
			adc_start_conversion();		
				
		}
	}
	else
		knobFlag=0;
	currSendButton=bit_is_clear(PINC,5);
	if (currSendButton && !prevSendButton && knobSize>0)
	{
		Usart_PutString("Sending:");
		knobSentence[knobSize++]='&';
		SIZE = knobSize;
		strcpy(i2cSendMessageBuf,knobSentence);
		syncStartFlag=0;
		TWCR = (1<<TWINT)|(1<<TWEN)| _BV(TWSTA)|(1<<TWEA)|(1<<TWIE);
		getCommand(knobSentence,knobSize);
		knobSize=0;
	}
	prevSendButton=currSendButton;

	if(configTimeoutCounter>200)
	{
		configTimeoutCounter=0;
		configAddressFlag=0;
		syncStartFlag=1;
		configStatingFlag=0;
	} 
	if(offFlag==0)
	{
		rightScrollingFlag=eeprom_read_byte(0x0002);
		columnScrollFlag=eeprom_read_byte(0x0004);
		if(!knobFlag)
		{
			sentenceSize=eeprom_read_byte(0x000A);
			if(sentenceSize>0 && sentenceSize<140)
			{
				if(columnScrollFlag && syncStartFlag)
				{
					scrollColumnTaskTime++;
				}
				ShowLetterTask();
				showColumnIndex++;
				if(showColumnIndex==5)
					showColumnIndex=0;
				if(scrollColumnTaskTime == scrollSpeed*8)    // 10ms passed, shifting the columns 
				{
					scrollSpeed=eeprom_read_byte(0x0006);
					ScrollColumnTask();
					scrollColumnTaskTime=0;
				} 
			}
		}
		else
		{
			knobShowLetter();
			knobIndex++;
			if(knobIndex==5)
				knobIndex=0;
		}
	}
}

ISR(TWI_vect)
{
	if(configAddressFlag)
	{
        devicePosition=interrupt_config_i2c();
       	initialPosition();
	}
	else
		interrupt_i2c();
}
 

void messageGettingUSART(char inputText[],char index)
{

	configAddressFlag=0;

	char i;
	for(i=2;i<index;i++)
	{
		eeprom_write_byte(0x0010+i-2,inputText[i]);
	}
	sentenceSize=index-2;
	eeprom_write_byte(0x000A,sentenceSize);
	sentenceSize=eeprom_read_byte(0x000A);
	eeprom_read_block(&sentence,0x0010,sentenceSize);
	showColumnIndex=0;

	//Giving initializing commands
	getCommand("09",2);//Stop
	initialPosition();//Initial position
	getCommand("06010",5);//Setting the speed as default

	getCommand("0A",2);//Return back to first
	getCommand("05",2);//Start shifting
	
	//TIMSK=0x10;
}
// ====================================== Hardware independent functions
char char2hex(char c)
{
	if(c>='0' && c<='9')
	{
		return (c-48);
	}
	else if(c>='A' && c<='F')
	{
		return (c-55);
	}
}
void userDefinesave(char inputText[])
{
	unsigned char charCode=0;
	unsigned char col=0;
	char i=4;
	char t=0;
	
	charCode=(char2hex(inputText[2]))*16+(char2hex(inputText[3]));
	while(i<14)
	{
		col=char2hex(inputText[i])*16;
		col+=char2hex(inputText[i+1]);
		eeprom_write_byte(0x00A0+charCode+t,col);
		i+=2;
		userDefine[charCode*5+t]=col;
		t++;
	}
	i=0;
	t=0;
}
void ScrollColumnTask()
{
	//sentenceSize=eeprom_read_byte(0x000A);
	if(rightScrollingFlag)
	{	
		columnIndex--;
		if(columnIndex<0)
		{
			columnIndex=5;
			sentenceLetterIndex--;
			if(sentenceLetterIndex<0)
				sentenceLetterIndex=(sentenceSize-1);
		}
	}
	else
	{
		columnIndex++;
		if(columnIndex>5)
		{
			columnIndex=0;
			sentenceLetterIndex++;
			if(sentenceLetterIndex>(sentenceSize-1))
				sentenceLetterIndex=0;
		}
	}
}
void ShowLetterTask()
{
	char portbValue;
	int sentenceValue,colShowSum;
	int nextLetter=0;
	sentenceValue=(sentence[sentenceLetterIndex]*5);
	colShowSum=columnIndex+showColumnIndex;
	//For fetching next letter in left shift when current letter is the last one 
	if(sentenceLetterIndex==(sentenceSize-1) && rightScrollingFlag==0)
		nextLetter=0;
	//For fetching next letter in left shift when current letter isn't the last one 
	else if(sentenceLetterIndex<(sentenceSize-1) && rightScrollingFlag==0)
		nextLetter=sentenceLetterIndex+1;
	
	//For fetching next letter in right shift when current letter is the last one 
	else if(rightScrollingFlag==1)
		nextLetter=sentenceLetterIndex+1;
		
	//For fetching next letter in right shift when current letter isn't the last one 
	if(sentenceLetterIndex==(sentenceSize-1) && rightScrollingFlag==1)
		nextLetter=0;

	if(colShowSum==5)
	{
		portbValue=0x00;
	}
	else if(colShowSum>5 && showColumnIndex<5)
	{
		if(sentence[nextLetter]<16)
			portbValue=userDefine[sentence[nextLetter]*5+(colShowSum-6)];
		else
			portbValue=pgm_read_byte(&(rowArray[(sentence[nextLetter]*5)+(colShowSum-6)]));
	}
	else
	{
		if(sentenceValue<16)
			portbValue=userDefine[sentenceValue+(colShowSum)];
		else
			portbValue=pgm_read_byte(&(rowArray[sentenceValue+colShowSum]));
	}
	PORTA=(PORTA&0xE0)|(colArray[showColumnIndex]&0x1F)|0b11100000;
	if(offFlag)
		PORTB=0x00;
	else
		PORTB=portbValue;
}
void getCommand(char inputText[],char index)
{
	int speed;
	configAddressFlag=0;
	switch(inputText[1])
	{
		case '1'://Right shift
			userDefinesave(inputText);
			break;
		case '2'://Right shift
			initialPosition();
			eeprom_write_byte(0x0002,1);
			ShowLetterTask();
			break;
		case '3'://left shift
			initialPosition();
			eeprom_write_byte(0x0002,0);
			ShowLetterTask();
			break;
		case '5'://Resume
			syncStartFlag=1;
			columnScrollFlag=1;
			eeprom_write_byte(0x0004,1);
			ShowLetterTask();
			break;
		case '6'://Speed
			syncStartFlag=0;
			speed=(inputText[2]-48)*100+(inputText[3]-48)*10+(inputText[4]-48);
			initialPosition();
			eeprom_write_word(0x0006,speed);
			syncStartFlag=1;
			break;

		case '8'://Downloading the message
			messageGettingUSART(inputText,index);
			ShowLetterTask();
			break;
		case '9'://stop
			syncStartFlag=0;
			columnScrollFlag=0;
			eeprom_write_byte(0x0004,0);
			ShowLetterTask();
			break;
		case 'A'://Return
			initialPosition();
			ShowLetterTask();
			break;
		case 'B'://Off
			offFlag=1;
			ShowLetterTask();
			break;
		case 'C'://On
			offFlag=0;
			ShowLetterTask();
			break;
		default:
			break;
	}

}
void loadingPreviousState(void)
{
	char i,j;
	sentenceSize=eeprom_read_byte(0x000A);
	eeprom_read_block((char *)&sentence,0x0010,sentenceSize);
	if(eeprom_read_byte(0x0002)!=0 && eeprom_read_byte(0x0002)!=1)
		eeprom_write_byte(0x0002,0);
	else
		rightScrollingFlag=eeprom_read_byte(0x0002);

	if(eeprom_read_byte(0x0004)!=0 && eeprom_read_byte(0x0004)!=1)
		eeprom_write_byte(0x0004,1);
	else
		columnScrollFlag=eeprom_read_byte(0x0004);
	if(eeprom_read_byte(0x0006)<10)
		eeprom_write_word(0x0006,scrollSpeed);
	//for(j=0;j<80;j++)
		//userDefine[j]=eeprom_read_byte(0x00A0+j);
}
void knobShowLetter(void)
{
	char temp;
	temp=pgm_read_byte(&(rowArray[knobColumnIndex*5+knobIndex]));
	PORTA=(PORTA&0xE0)|(colArray[knobIndex]&0x1F)|0b11100000;
	PORTB=temp;
}

void initialPosition()
{
	
	if(devicePosition==1)
    {
		columnIndex=0;
        sentenceLetterIndex=0;
           
    }
	else if(devicePosition==2)
    {
		columnIndex=5;
   		sentenceLetterIndex=0;
    }
	else if(devicePosition>2)
    {
		columnIndex=(5-((int)(devicePosition)-2)%5);
        sentenceLetterIndex=(int16_t)devicePosition-2;
           
    }
 }

//======================================= Main function
int main(void)
{
	
	initHardware();
	PORTC &=0b11110111;
	PORTD &=0b11110111;
	//Waiting for 2 seconds to let the pins on the board be stable
	_delay_ms(2000);
	loadingPreviousState();
	//Finiding the first master for starting the configuration
	if(!bit_is_clear(PINC,3))
		TWCR = (1<<TWINT)|(1<<TWEN)| _BV(TWSTA)|(1<<TWEA)|(1<<TWIE);// master
	else
		TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA)|(1<<TWIE);

	configStatingFlag=1;
	sei();

	while(1);			
	return 0;
}
