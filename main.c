#include <msp430g2553.h>
#include "LCD.h" 

#define STATE_INIT      0
#define STATE_COUNTDOWN 1
#define STATE_WAIT      2
#define STATE_MEASURE   3
#define STATE_DONE      4

volatile unsigned long time_ticks = 0;    
volatile unsigned int is_running = 0;     
volatile unsigned int update_display = 1;
volatile unsigned int button_pressed = 0; // Check if the button has been pressed or not

unsigned long reaction_times[10];         // Array of registers that stores the value of the time
unsigned int test_count = 0;
unsigned int avg = 0;
unsigned int sum = 0;

void Takeavg();
{

}

void main(void)
{
    WDTCTL  = WDTPW | WDTHOLD;      

    DCOCTL = 0;
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    initLCD(); 

    // LEDs
    P1DIR |= BIT0 | BIT6;           
    P1OUT &= ~(BIT0 | BIT6);  

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
            printString("Hit button to start");
            button_pressed = 0; 
            while(!button_pressed); // This is a infinite loop that ensures the button being pressed
            current_state = STATE_COUNTDOWN;
        }
        
        else if (current_state == STATE_COUNTDOWN) {
            printString("3");
            __delay_cycles(1000000); // 1 second delay
            printString("2");
            __delay_cycles(1000000); 
            printString("1");
            __delay_cycles(1000000); 
            printString("GO!");
            __delay_cycles(500000); // Half a second
            
            test_count = 0; // Reset test counter
            current_state = STATE_WAIT;
        }
        
        else if (current_state == STATE_WAIT) {
            printString("Wait for Green..");
            
            // Wait 3 seconds before turning on LED
            __delay_cycles(3000000); 
            
            button_pressed = 0; // Clear any accidental early presses
            current_state = STATE_MEASURE;
        }
        
        else if (current_state == STATE_MEASURE) {
            P1OUT |= BIT6;      // Turn ON Green LED (P1.6)
            
            time_ticks = 0;     // Reset stopwatch
            is_running = 1;     // Start stopwatch
            
            while(!button_pressed) { // Shows time 
                printTime(time_ticks);
            }
            
            is_running = 0;     // Stop stopwatch
            P1OUT &= ~BIT6;     // Turn OFF Green LED
            
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
            printString("Done, calc...");
            
            int i;
            for(i = 0; i < 10; i++) {
                P1OUT ^= (BIT0 | BIT6); // Toggle P1.0 and P1.6
                __delay_cycles(200000); // Fast blink delay
            }
            P1OUT &= ~(BIT0 | BIT6);  
            
            sum = 0;
            for(i = 0; i < 10; i++) {
                sum += reaction_times[i];
            }
            avg = sum / 10;
            
            // Display final average
            printString("Avg Reaction:");
            printAvg(avg); 
            __delay_cycles(10000000) // Delay 10s
            button_pressed = 0;
            printString("Press button to restart")
            while(!button_pressed); 
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
