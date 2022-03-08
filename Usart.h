#include <avr\sleep.h> 
#include <util\delay.h>
#include <stdio.h>
#include <avr\io.h>
#define F_CPU 3686400
#define BAUD 9600//calculate UBRR value

void USART_Init(); // init the usart
unsigned char USART_Receive( void );// Receiving the text from USART
void USART_Transmit( unsigned char data );// Sending by USART
void Usart_PutChar(unsigned char cTXData);
void Usart_PutString(unsigned char *pcString);