#ifndef LCD_H_
#define LCD_H_

#include <msp430g2553.h>

#define RS_PIN      BIT0    // P2.0 -> RS
#define E_PIN       BIT1    // P2.1 -> E
// Port 1 for displaying (P1.0 -> P1.7)
#define DATA_DIR    P1DIR   // Port 1 Direction
#define DATA_OUT    P1OUT   // Port 1 Output
// Port 2 for instructing the LCD (P2.0, P2.1) 
#define CTRL_DIR    P2DIR   // Port 2 Direction
#define CTRL_OUT    P2OUT   // Port 2 Output

void pulseEnable(void);
void writeCommand(unsigned char command);
void writeChar(char data);
void printString(char* text);
void initLCD(void);
void clearLCD(void);
void printAvg(unsigned long ticks);
void printTime(unsigned long ticks);
void delay_us(unsigned int us);
void delay_ms(unsigned int ms);

#endif 