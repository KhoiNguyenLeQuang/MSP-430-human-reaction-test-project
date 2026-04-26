#include <msp430g2553.h>
#include "lcd.h" 

#define STATE_INIT      0
#define STATE_COUNTDOWN 1
#define STATE_WAIT      2
#define STATE_MEASURE   3
#define STATE_DONE      4

volatile unsigned long time_ticks = 0;    
volatile unsigned int is_running = 0;     
volatile unsigned int update_display = 1;
volatile unsigned int button_pressed = 0; 

unsigned int current_state = 0;
unsigned long reaction_times[10];         
unsigned int test_count = 0;

// New Variables for Go/No-Go and Penalties
unsigned int is_go_trial = 1; 
unsigned long penalty_ticks = 0; // 1 tick = 10ms
unsigned long avg = 0;
unsigned long sum = 0;

// UART Helper Functions
void uart_print(char *str) {
    while (*str != '\0') {
        while (!(IFG2 & UCA0TXIFG)); // Wait for TX buffer to be ready
        UCA0TXBUF = *str;
        str++;
    }
}

void uart_print_num(unsigned long num) {
    char buf[10];
    int i = 0;
    if (num == 0) { uart_print("0"); return; }
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    while (i > 0) {
        while (!(IFG2 & UCA0TXIFG));
        UCA0TXBUF = buf[--i];
    }
}

void sendDataCSV() {
    // 1. Send CSV Header
    uart_print("Trial,ReactionTime_ms\r\n");

    // 2. Loop through array and send data
    int i;
    for (i = 0; i < 10; i++) {
        uart_print_num(i + 1);
        uart_print(",");
        uart_print_num(reaction_times[i] * 10); // Convert ticks to milliseconds
        uart_print("\r\n");
    }
    
    // 3. Send final penalty data
    uart_print("Total Penalty_ms,");
    uart_print_num(penalty_ticks * 10);
    uart_print("\r\n\r\n");
}

void main(void)
{
    WDTCTL  = WDTPW | WDTHOLD;      

    DCOCTL = 0;
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    initLCD(); 

    // LEDs (P2.4 is RED, P2.5 is GREEN)
    P2DIR |= BIT4 | BIT5;           
    P2OUT &= ~(BIT4 | BIT5);  

    // P2.3 button
    P2DIR &= ~BIT3;                 
    P2OUT |= BIT3;                  
    P2REN |= BIT3;                  
    P2IE  |= BIT3;                  
    P2IES |= BIT3;                 
    P2IFG &= ~BIT3;                 

    // STOPWATCH TIMER SETUP (Timer 1)
    TA1CCR0 = 1250 - 1;             
    TA1CCTL0 = CCIE;                
    TA1CTL = TASSEL_2 | ID_3 | MC_1 | TACLR; 

    // UART HARDWARE SETUP 
    P1SEL |= BIT1 | BIT2;     // Set P1.1 and P1.2 to UART RX/TX mode
    P1SEL2 |= BIT1 | BIT2;
    
    UCA0CTL1 |= UCSSEL_2;     // Use SMCLK (1MHz)
    UCA0BR0 = 104;            // 1MHz / 9600 = 104
    UCA0BR1 = 0;              
    UCA0MCTL = UCBRS0;        // Modulation UCBRSx = 1
    UCA0CTL1 &= ~UCSWRST;     // Initialize UART state machine

    _BIS_SR(GIE);             

    // MAIN LOOP
    while(1) {
        if (current_state == STATE_INIT) {
            clearLCD();
            printString("Hit button to   start");
            button_pressed = 0; 
            while(!button_pressed); 
            current_state = STATE_COUNTDOWN;
        }
        
        else if (current_state == STATE_COUNTDOWN) {
            clearLCD();
            printString("Wait for green. DO NOT hit red.");
            delay_ms(3000); 
            clearLCD(); printString("3"); delay_ms(1000); 
            clearLCD(); printString("2"); delay_ms(1000); 
            clearLCD(); printString("1"); delay_ms(1000); 
            clearLCD(); printString("GO!"); delay_ms(500); 
            clearLCD();
            
            test_count = 0; 
            penalty_ticks = 0; // Reset penalties
            current_state = STATE_WAIT;
        }
        
        else if (current_state == STATE_WAIT) {
            if (test_count == 0) {
                delay_ms(3000);              
            }
            else {
                unsigned long previous_time = reaction_times[test_count - 1]; 
                unsigned long dynamic_delay_ms = (test_count * (previous_time * 10)) + 2000;
                delay_ms(dynamic_delay_ms);
            }
            
            is_go_trial = (TA1R % 3) != 0; // 66% chance of Green, 33% Red

            button_pressed = 0; 
            current_state = STATE_MEASURE;
        }
        
        else if (current_state == STATE_MEASURE) {
            time_ticks = 0;     
            is_running = 1;     
            
            if (is_go_trial) {
                // GO TRIAL (GREEN LED)
                P2OUT |= BIT5; 
                
                while(!button_pressed) { 
                    if (update_display == 1) {
                        printTime(time_ticks);
                        update_display = 0;
                    }
                }
                is_running = 0;     
                P2OUT &= ~BIT5;     
                
                reaction_times[test_count] = time_ticks; 
                test_count++;
            } 
            else {
                // NO-GO TRIAL (RED LED)
                P2OUT |= BIT4; 
                clearLCD();
                printString("HOLD...");
                
                // Wait 2 seconds (200 ticks) to see if they accidentally press it
                while(time_ticks < 200 && !button_pressed);
                
                is_running = 0;
                P2OUT &= ~BIT4; 

                clearLCD();
                if (button_pressed) {
                    printString("FAILED! +500ms");
                    penalty_ticks += 50; // Add 500ms penalty
                } else {
                    printString("Good hold!");
                }
                delay_ms(2000);
            }
            
            if (test_count < 10) {
                current_state = STATE_WAIT; 
            } else {
                current_state = STATE_DONE; 
            }
        }
        
        else if (current_state == STATE_DONE) {
            int i;
            for(i = 0; i < 10; i++) {
                P2OUT ^= (BIT4 | BIT5); 
                delay_ms(200); 
            }
            P2OUT &= ~(BIT4 | BIT5);  
            
            // Calculate Average WITH Penalties included
            sum = 0;
            for(i = 0; i < 10; i++) {
                sum += reaction_times[i];
            }
            avg = (sum + penalty_ticks) / 10;
            
            clearLCD();
            printAvg(avg); 
            
            // FIRE THE DATA PIPELINE
            sendDataCSV();
            
            delay_ms(10000); 
            clearLCD();
            button_pressed = 0;
            printString("Press button to restart");
            while (!button_pressed);
            current_state = STATE_INIT;
        }
    }
}

// INTERRUPT SERVICE ROUTINES
#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer1_A0(void) {
    if (is_running == 1) {
        time_ticks++;       
        update_display = 1; 
    }
}

#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void) {
    unsigned long delay;                                
    for(delay=0 ; delay<12345 ; delay=delay+1); // Debounce delay
    
    // If button pressed 
    if (P2IFG & BIT3) { 
      button_pressed = 1;  
      P2IFG &= ~BIT3;      
    }
}
