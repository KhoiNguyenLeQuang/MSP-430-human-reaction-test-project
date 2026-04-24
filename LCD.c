#include "lcd.h"
#include <msp430g2553.h> 

void delay_us(unsigned int us) {
    TA0CTL = TACLR;                 
    TA0CCR0 = us;                   
    TA0CTL = TASSEL_2 | MC_1;      
    
    while ((TA0CTL & TAIFG) == 0);  
    
    TA0CTL &= ~TAIFG;               
    TA0CTL = MC_0;                
}

void delay_ms(unsigned int ms) {
    while (ms > 0) {
        delay_us(1000); 
        ms--;
    }
}

void pulseEnable(void) {
    // Step 3: Prime the Clock
    CTRL_OUT |= E_PIN;        
    delay_us(1);             
    // Step 5: Clock it in (create a falling edge)
    CTRL_OUT &= ~E_PIN;      
    delay_ms(2);             
}

void writeCommand(unsigned char command) {
    // Step 1: Set the Mode (RS LOW for Command)
    CTRL_OUT &= ~RS_PIN;       
    // Step 2: Sends the command to the LCD's instruction register
    DATA_OUT = command;         
    pulseEnable();
}

void writeChar(char data) {
    // Step 1: Set the Mode (RS HIGH for Text)
    CTRL_OUT |= RS_PIN;     
    // Step 2: Sends the data to the LCD screen
    DATA_OUT = data;         
    pulseEnable();
}

void printString(char* text) {
    unsigned char char_count = 0; 

    while (*text != '\0') {
        if (char_count == 16) {
            writeCommand(0xC0); 
        }
        if (char_count >= 32) {
            break; 
        }
        writeChar(*text);
        char_count++;
        text++;
    }
}

void initLCD(void) {
    DATA_DIR = 0xFF;                  
    CTRL_DIR |= (RS_PIN | E_PIN);     
    // Ensure both E and RS start LOW before we do anything
    CTRL_OUT &= ~(RS_PIN | E_PIN);
    
    delay_ms(20);
    
    writeCommand(0x38);      // 8-bit mode, 2 lines
    writeCommand(0x0C);      // Display ON, Cursor OFF
    writeCommand(0x01);      // Clear Screen
    writeCommand(0x06);      // Auto-increment cursor
}

void clearLCD(void) {
    writeCommand(0x01);
}

void printTime(unsigned long ticks) {
    unsigned int mins = (ticks / 6000);       // Calculate minutes
    unsigned int secs = (ticks / 100) % 60;   // Calculate seconds
    unsigned int hund = ticks % 100;          // Calculate hundredths of a second
    
    char buffer[9];
    buffer[0] = (mins / 10) + '0';
    buffer[1] = (mins % 10) + '0';
    buffer[2] = ':';
    buffer[3] = (secs / 10) + '0';
    buffer[4] = (secs % 10) + '0';
    buffer[5] = '.';
    buffer[6] = (hund / 10) + '0';
    buffer[7] = (hund % 10) + '0';
    buffer[8] = '\0';
    
    writeCommand(0x80); 
    printString("Time:      ");
    writeCommand(0xC0); 
    printString(buffer);
    printString("   "); 
}

void printAvg(unsigned long ticks) {
    unsigned int mins = (ticks / 6000);       // Calculate minutes
    unsigned int secs = (ticks / 100) % 60;   // Calculate seconds
    unsigned int hund = ticks % 100;          // Calculate hundredths of a second
    
    char buffer[9];
    buffer[0] = (mins / 10) + '0';
    buffer[1] = (mins % 10) + '0';
    buffer[2] = ':';
    buffer[3] = (secs / 10) + '0';
    buffer[4] = (secs % 10) + '0';
    buffer[5] = '.';
    buffer[6] = (hund / 10) + '0';
    buffer[7] = (hund % 10) + '0';
    buffer[8] = '\0';
    
    writeCommand(0x80); 
    printString("Average Time:      ");
    writeCommand(0xC0); 
    printString(buffer);
    printString("   "); 
}