#include "i2c.h"
#include <avr/io.h>

//volatile unsigned char messageBuf[8]="WELCOM!";
//volatile unsigned char recMsg[150]={};
static volatile unsigned char address = 0x01;
volatile unsigned char maxAddress = 0x00;

//volatile unsigned char masterFlag = 0;
unsigned char volatile  masterCount = 0;
extern  volatile unsigned char configTimeoutCounter=0;
unsigned char volatile  LeftFlag = 0;
volatile unsigned char LWPinDelayCounter=0;
volatile unsigned char indexSen= 0;
volatile unsigned char indexRec= 0;
extern volatile char configStatingFlag; 

unsigned char volatile i2cSendMessageBuf[];
unsigned char volatile SIZE=0;
unsigned char volatile i2cReceiveMessageBuf[];
extern volatile int scrollColumnTaskTime;
extern volatile char syncStartFlag;


void InitI2C(void)
{
	TWAR = 0x01; // Slave Address
	TWBR = 10; // Bit rate = 100Khz
	TWCR = 0xC5; 
}

char interrupt_config_i2c(void)
{
  
	unsigned char statue;
	extern volatile unsigned char LWPinDelayCounter;
	if(!bit_is_clear(PINC,3))//PIND&(0b00000100))
 		LeftFlag=1;
	/*	Usart_PutString("Left Pin is 1");
		Usart_PutString("The leftFlag value is :");
		Usart_PutChar(LeftFlag+0x30);
	}
	else
		Usart_PutString("Left Pin is 0");
        Usart_PutChar(0x0d);
        Usart_PutChar(0x0a);*/

/*  Usart_PutString("Left pin is : ");
  Usart_PutChar(LeftFlag+0x30);
  Usart_PutChar(0x0d);
  Usart_PutChar(0x0a);*/

	statue = TW_STATUS;
	switch(statue)                             
	{
    // master only once by the left pins B0 is 1 
	// the master test 		
		case TW_START :
		       /* Usart_PutString("Master START");
				Usart_PutChar(0x0d);
				Usart_PutChar(0x0a);*/
				configStatingFlag=1;
				TWDR = 0x00;
   			    TWCR = _BV(TWINT) | _BV(TWEN)|(1<<TWIE)| (1<<TWEA);
				break;
	    case TW_MT_SLA_ACK:
		        /*Usart_PutString("Master START");
                Usart_PutChar(0x0d);
                Usart_PutChar(0x0a);
				  Usart_PutString("I put 1 on my right, my right pin is:");
				  Usart_PutChar(0x0d);
				  Usart_PutChar(0x0a);*/
			    PORTD |=0b00001000;
		   		TWDR = address;			
				TWCR = _BV(TWINT) | _BV(TWEN)|(1<<TWIE)| (1<<TWEA);	
				break;
		case TW_MT_DATA_ACK:
				/*Usart_PutString("data ack");
                Usart_PutChar(0x0d);
                Usart_PutChar(0x0a);*/
		        masterCount++;
			    TWCR = (1<<TWINT) | _BV(TWSTO) | (1<<TWEN)|(1<<TWIE) | (1<<TWEA);	// switch it to slave
				configTimeoutCounter= 0;
				break;
		case TW_REP_START:
		case TW_MT_SLA_NACK:
		case TW_MT_DATA_NACK:
				/*Usart_PutString("address not ack");
                Usart_PutChar(0x0d);
                Usart_PutChar(0x0a);*/
				TWCR = _BV(TWINT) | _BV(TWEN)|(1<<TWIE)| _BV(TWSTA)| (1<<TWEA);
				break;
	    //   The slave part 
        //  check the left pin , if it is 1, switch it to master
	
		case TW_SR_GCALL_ACK:
				/*Usart_PutString("Receive the Genel address broadcase");
                Usart_PutChar(0x0d);
                Usart_PutChar(0x0a);*/
				TWCR=_BV(TWEA) | _BV(TWINT)|(1<<TWIE)| _BV(TWEN);
				break; 
        case TW_SR_GCALL_DATA_ACK:
   				/*Usart_PutString("Receive the Genel data ack broadcase");
                Usart_PutChar(0x0d);
                Usart_PutChar(0x0a);*/
				if(LeftFlag == 1 && masterCount<1)
				    address = TWDR+1;
				else if (maxAddress<TWDR)
				    maxAddress = TWDR;
				configTimeoutCounter= 0;
				TWCR=_BV(TWEA) | _BV(TWINT)|(1<<TWIE)| _BV(TWEN);
				break;			
		case TW_SR_STOP:
				/*Usart_PutString("STOP");
                Usart_PutChar(0x0d);
                Usart_PutChar(0x0a);*/
				if(LeftFlag == 1 && masterCount<1){
					configTimeoutCounter= 0;
					TWCR=_BV(TWEA) |_BV(TWINT)|(1<<TWIE) |_BV(TWSTA)| _BV(TWEN);//switch to master
				}
				else
				   TWCR=(1<<TWEA)| _BV(TWINT)|(1<<TWIE)| _BV(TWEN);// still is slave
				break;
		case TW_SR_SLA_ACK:    
		case TW_SR_DATA_ACK:
		case TW_SR_ARB_LOST_GCALL_ACK:		
		case TW_SR_GCALL_DATA_NACK:
	    default:
				TWCR = _BV(TWINT) |(1<<TWEA)|(1<<TWIE)| _BV(TWEN);
				break;
   }
	if(maxAddress <= address)
	    maxAddress = address;
   
	Usart_PutString("my address is ");
	Usart_PutChar(address+0x30);
	Usart_PutChar(0x0d);
    Usart_PutChar(0x0a);
	
	Usart_PutString("max address is ");
    Usart_PutChar(maxAddress+0x30);
	Usart_PutChar(0x0d);
    Usart_PutChar(0x0a);
	return address;
}
void interrupt_i2c()
{
    unsigned char statue;
    statue=TW_STATUS;
	switch(statue)
	{   
			// the master test 		
            	case TW_START :
				        indexSen=0;	
						TWDR = 0x00;
	                    TWCR = _BV(TWINT) | _BV(TWEN)|(1<<TWIE)| (1<<TWEA);
						//TWCR = _BV(TWINT) | _BV(TWEN)|(1<<TWIE);
						break;
				case TW_MT_SLA_ACK:
				        TWDR = 0x00;
	                    TWCR = _BV(TWINT) | _BV(TWEN)|(1<<TWIE)| (1<<TWEA);
					    //TWCR = _BV(TWINT) | _BV(TWEN)|(1<<TWIE);
						break;
			    case TW_REP_START:
						TWCR = _BV(TWINT) | _BV(TWEN)|(1<<TWIE)| _BV(TWSTA)| (1<<TWEA);
						//TWCR = _BV(TWINT) | _BV(TWEN)|(1<<TWIE)| _BV(TWSTA);
						break;
				case TW_MT_DATA_NACK:
						TWCR = _BV(TWINT) | _BV(TWEN)|(1<<TWIE)| _BV(TWSTA)| (1<<TWEA);
						break;
				case TW_MT_SLA_NACK:
						TWCR = _BV(TWINT) | _BV(TWEN)|(1<<TWIE)| _BV(TWSTA)| (1<<TWEA);
						break;
				case TW_MT_DATA_ACK:
						if(indexSen < SIZE){
						     // TWDR = messageBuf[indexSen];
							  TWDR = i2cSendMessageBuf[indexSen];
							  /*Usart_PutString("Sending TWDR is : ");
							  Usart_PutChar(TWDR);
							  Usart_PutChar(0x0d);
                              Usart_PutChar(0x0a);*/
							  indexSen++;
					          TWCR = _BV(TWINT) | _BV(TWEN)|(1<<TWIE) |(1<<TWEA);
                           }
						else
						{
						   // Usart_PutChar(0x0d);
                            //Usart_PutChar(0x0a);
							//Usart_PutString("Message sent finished, My letter is: ");
							//Usart_PutChar(messageBuf[address]);
							//Usart_PutChar(0x0d);
                            //Usart_PutChar(0x0a);
							//initialPosition();
							//scrollColumnTaskTime=50;
							
							//initialPosition();
							
							syncStartFlag=1;
						    TWCR = (1<<TWINT) | _BV(TWSTO) | (1<<TWEN)|(1<<TWIE)|(1<<TWEA);                  

                        } 					
						
						break;
				//   The slave part 
                case TW_SR_GCALL_ACK:
    				      TWCR=_BV(TWEA) | _BV(TWINT)|(1<<TWIE)| _BV(TWEN);
					  break;
		        case TW_SR_GCALL_DATA_ACK:
					  /*if(indexRec <SIZE){
					        if(indexRec>0){*/
							   // recMsg[indexRec-1] = TWDR;
							   i2cReceiveMessageBuf[indexRec-1] = TWDR;
								//Usart_PutChar(TWDR);
							//}
							
						/*}
					  else
					  {
				        Usart_PutChar(0x0d);
                        Usart_PutChar(0x0a);
					   }*/
					//Usart_PutString("Received TWDR is : ");
					//Usart_PutChar(i2cReceiveMessageBuf[indexRec]);
					//Usart_PutChar(0x0d);
					//Usart_PutChar(0x0a);
					indexRec++;
					TWCR=_BV(TWEA) | _BV(TWEN)|_BV(TWINT)|(1<<TWIE);
					break;
				case TW_SR_STOP:
						/*Usart_PutString("STOP");
						Usart_PutChar(0x0d);
                        Usart_PutChar(0x0a);*/
						indexRec--;
						//scrollColumnTaskTime=0;
						syncStartFlag=1;
						//initialPosition();
						getCommand(i2cReceiveMessageBuf,indexRec);
						indexRec= 0;
						/*Usart_PutString("Received message:");
						Usart_PutString(i2cReceiveMessageBuf);
						Usart_PutChar(0x0d);
                        Usart_PutChar(0x0a);*/
						TWCR=_BV(TWEA) | _BV(TWEN)|_BV(TWINT)|(1<<TWIE);
						
						break;
				case TW_SR_GCALL_DATA_NACK:	  
				case TW_SR_ARB_LOST_GCALL_ACK:	
				case TW_SR_SLA_ACK:    
				case TW_SR_DATA_ACK:
				default:
						TWCR = _BV(TWINT) | _BV(TWEN)|(1<<TWEA)|(1<<TWIE);// set the stop bit
						break;
	}
}