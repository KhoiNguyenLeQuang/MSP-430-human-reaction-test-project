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
    CTRL_OUT |= E_PIN;        
    delay_us(1);             
    CTRL_OUT &= ~E_PIN;      
    delay_ms(2);             
}

// 4-bit Data Function
void sendNibble(unsigned char nibble) {
    DATA_OUT &= 0x0F;                // Clear the upper 4 bits (P1.4 - P1.7)
    DATA_OUT |= (nibble & 0xF0);     // Set the upper 4 bits with data
    pulseEnable();
}

void writeCommand(unsigned char command) {
    CTRL_OUT &= ~RS_PIN;             // RS LOW for Command
    sendNibble(command);             // Send High Nibble (Top 4 bits)
    sendNibble(command << 4);        // Send Low Nibble (Bottom 4 bits shifted up)
}

void writeChar(char data) {
    CTRL_OUT |= RS_PIN;              // RS HIGH for Text
    sendNibble(data);                // Send High Nibble
    sendNibble(data << 4);           // Send Low Nibble
}

// Upgraded Print String with 16-character auto line-break
void printString(char* text) {
    unsigned char char_count = 0; 
    while (*text != '\0') {
        if (char_count == 16) {
            writeCommand(0xC0);      // Jump to line 2
        }
        if (char_count >= 32) {
            break;                   // Stop printing if screen is full
        }
        writeChar(*text);
        char_count++;
        text++;
    }
}

void initLCD(void) {
    DATA_DIR |= 0xF0;                 // Set only P1.4 to P1.7 as outputs
    CTRL_DIR |= (RS_PIN | E_PIN);     
    CTRL_OUT &= ~(RS_PIN | E_PIN);
    
    delay_ms(20);
    
    // Strict 4-bit wake-up sequence
    CTRL_OUT &= ~RS_PIN;
    sendNibble(0x30);
    delay_ms(5);
    sendNibble(0x30);
    delay_ms(1);
    sendNibble(0x30);
    delay_ms(1);
    sendNibble(0x20);        
    
    writeCommand(0x28);      // 4-bit mode, 2 lines, 5x8 font
    writeCommand(0x0C);      // Display ON, Cursor OFF
    writeCommand(0x01);      // Clear Screen
    writeCommand(0x06);      // Auto-increment cursor
}

void clearLCD(void) {
    writeCommand(0x01);
}

void setCursor(unsigned char row, unsigned char col) {
    unsigned char address = (row == 0) ? 0x80 : 0xC0;
    address |= col;
    writeCommand(address);
}

// Timer Functions from Activity 24
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
    unsigned int mins = (ticks / 6000);       
    unsigned int secs = (ticks / 100) % 60;   
    unsigned int hund = ticks % 100;          
    
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
    printString("Avg Time:       ");  
    writeCommand(0xC0); 
    printString(buffer);
    printString("   "); 
}
