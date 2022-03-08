#include "Usart.h"

void USART_Init()
{
	UCSRC=(1<<URSEL)|(0<<UMSEL)|(0<<UPM1)|(0<<UPM0)|(0<<USBS)|(0<<UCSZ2)|(1<<UCSZ1)|(1<<UCSZ0);
	//Enable Transmitter and Receiver and Interrupt on receive complete	
	UCSRA = 0X00;    
	UCSRB=(1<<RXEN)|(1<<TXEN)|(1<<RXCIE);	
	//enable global interrupts	
	UBRRL = (F_CPU / BAUD / 16 - 1) % 256;
	UBRRH = (F_CPU / BAUD / 16 - 1) / 256;        
}

unsigned char USART_Receive( void )
{
    /* Wait for data to be received */
    while ( !(UCSRA & (1<<RXC)) );
	//masterFlag=0;
    /* Get and return received data from buffer */
    return UDR;
}
void USART_Transmit( unsigned char data )
{
/* Wait for empty transmit buffer */
    while ( !( UCSRA & (1<<UDRE)) );
/* Put data into buffer, sends the data */
    UDR = data;
}
void Usart_PutChar(unsigned char cTXData)
{
	while( !(UCSRA & (1 << UDRE)) );  
	UDR = cTXData;                   

}
void Usart_PutString(unsigned char *pcString)
{
	while (*pcString)

	{
		Usart_PutChar(*pcString++);   

	}
}