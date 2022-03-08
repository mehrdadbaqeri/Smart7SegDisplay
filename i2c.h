#include "Usart.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/twi.h>


#define uint unsigned int
#define uchar unsigned char


void InitI2C(void);
void interrupt_i2c(void);
char interrupt_config_i2c(void);