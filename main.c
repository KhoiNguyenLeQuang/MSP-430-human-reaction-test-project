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
volatile unsigned int button_pressed = 0; // Check if the button has been pressed or not

unsigned int current_state = 0;
unsigned long reaction_times[10];         // Array of registers that stores the value of the time
unsigned int test_count = 0;
unsigned long avg = 0;
unsigned long sum = 0;

void main(void)
{
    WDTCTL  = WDTPW | WDTHOLD;      

    DCOCTL = 0;
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    initLCD(); 

    // LEDs
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
    TA1CCTL0 = CCIE;                // Enable Timer1_A0 interrupt
    TA1CTL = TASSEL_2 | ID_3 | MC_1 | TACLR; // SMCLK, Div 8, Up mode, Clear timer

    _BIS_SR(GIE);                   // Activate all interrupts



    // MAIN LOOP
while(1) {
        if (current_state == STATE_INIT) {
            clearLCD();
            printString("Hit button to   start");
            button_pressed = 0; 
            while(!button_pressed); // This is a infinite loop that ensures the button being pressed
            current_state = STATE_COUNTDOWN;
        }
        
        else if (current_state == STATE_COUNTDOWN) {
            clearLCD();
            printString("Wait for green  light to turn on");
            delay_ms(5000); // 5 second delay
            clearLCD();
            printString("3");
            delay_ms(1000); // 1 second delay
            clearLCD();
            printString("2");
            delay_ms(1000); 
            clearLCD();
            printString("1");
            delay_ms(1000); 
            clearLCD();
            printString("GO!");
            delay_ms(500); // Half a second
            clearLCD();
            
            test_count = 0; // Reset test counter
            current_state = STATE_WAIT;
        }
        
        else if (current_state == STATE_WAIT) {
            if (test_count == 0) {
            // Wait 3 seconds before turning on LED
            delay_ms(3000);              
            }
            else {
                // This took the previous measurement multiply by the current test count and add 4s to it
                unsigned long previous_time = reaction_times[test_count - 1]; // the value of previous register.
                unsigned long dynamic_delay_cycles = (test_count * previous_time) + 4000;
                delay_ms(dynamic_delay_cycles);
                ;
            }
            
            button_pressed = 0; 
            current_state = STATE_MEASURE;
        }
        
        else if (current_state == STATE_MEASURE) {
            P2OUT |= BIT5;      // Turn ON external Green LED (P2.5)
            
            time_ticks = 0;     // Reset stopwatch
            is_running = 1;     // Start stopwatch
            
            while(!button_pressed) { 
                if (update_display == 1) {
                printTime(time_ticks);
                }
            }

            is_running = 0;     // Stop stopwatch
            P2OUT &= ~BIT5;     // Turn OFF Green LED
            
            // Save time to the array (the "different registers")
            reaction_times[test_count] = time_ticks; 
            test_count++;
            
            if (test_count < 10) {
                current_state = STATE_WAIT; // Loop back for next sequence
            } else {
                current_state = STATE_DONE; // 10 tests are finished!
            }
        }
        
        else if (current_state == STATE_DONE) {
            int i;
            for(i = 0; i < 10; i++) {
                P2OUT ^= (BIT4 | BIT5); // Toggle P2.4 and P2.5
                delay_ms(500); // Fast blink delay
            }
            P2OUT &= ~(BIT4 | BIT5);  
            
            sum = 0;
            for(i = 0; i < 10; i++) {
                sum += reaction_times[i];
            }
            avg = sum / 10;
            
            // Display final average
            printAvg(avg); 
            delay_ms(10000); // Delay 10s
            clearLCD();
            button_pressed = 0;
            printString("Press button to restart");
            while (!button_pressed);
            current_state = STATE_INIT;
        }
    }
}

// INTERRUPT SERVICE ROUTINE
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
    for(delay=0 ; delay<12345 ; delay=delay+1); 
    // If button pressed 
    if (P2IFG & BIT3) { 
      button_pressed = 1;  
      P2IFG &= ~BIT3;      
    }
}