#include "lcd.h"
#include <msp430g2553.h> 

void delay_us(unsigned int us) {
    while (us--) {
        __delay_cycles(1); 
    }
}

void delay_ms(unsigned int ms) {
    while (ms--) {
        __delay_cycles(1000); 
    }
}

void pulseEnable(void) {
    CTRL_OUT |= E_PIN;
    __delay_cycles(1);
    CTRL_OUT &= ~E_PIN;
    __delay_cycles(50); 
}

void writeCommand(unsigned char command) {
    CTRL_OUT &= ~RS_PIN;

    DATA_OUT = command;  

    pulseEnable();

    if (command == 0x01 || command == 0x02) {
        delay_ms(2);
    }
}

void writeChar(char data) {
    CTRL_OUT |= RS_PIN;

    DATA_OUT = data;

    pulseEnable();
}

void printString(char* text) {
    unsigned char addr = 0;
    unsigned int count = 0;

    while (*text != '\0') {

        if (count == 16) {
            writeCommand(0xC0); 
        }

        if (count >= 32) {
            break;
        }

        writeChar(*text);
        text++;
        count++;
    }
}

void printStringWrapped(char* text) {
    unsigned int count = 0;

    while (*text != '\0' && count < 32) {

        if (count == 16) {
            writeCommand(0xC0); 
        }

        writeChar(*text);
        text++;
        count++;
    }
}

void initLCD(void) {
    DATA_DIR = 0xFF;                  
    CTRL_DIR |= (RS_PIN | E_PIN);     
    CTRL_OUT &= ~(RS_PIN | E_PIN);
    
    delay_ms(20);
    
    writeCommand(0x38);      // 8-bit mode, 2 lines
    writeCommand(0x0C);      // Display ON, Cursor OFF
    writeCommand(0x01);      // Clear Screen
    writeCommand(0x06);      // Auto-increment cursor
}

void clearLCD(void) {
    writeCommand(0x01);
    delay_ms(2);
}

void printTime(unsigned long ticks) {
    unsigned int mins = ticks / 60000;
    unsigned int secs = (ticks / 1000) % 60;
    unsigned int hund = (ticks/10) % 100;

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

    writeCommand(0xC0 + 4);
    printString(buffer);
}

void printAvg(unsigned long ticks) {
    unsigned int mins = ticks / 60000;
    unsigned int secs = (ticks / 1000) % 60;
    unsigned int hund = (ticks/10) % 100;

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
    printString("  Final Average ");

    writeCommand(0xC0 + 4);
    printString(buffer);
}

void printError(unsigned long pct) {
    char buf[5];

    buf[0] = (pct / 100) + '0';
    buf[1] = ((pct / 10) % 10) + '0';
    buf[2] = (pct % 10) + '0';
    buf[3] = '%';
    buf[4] = '\0';

    writeCommand(0x80);
    printString("   Avg Error:   ");

    writeCommand(0xC0 + 6);
    printString(buf);
}
