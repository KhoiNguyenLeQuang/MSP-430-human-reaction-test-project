#include <msp430g2553.h>
#include "lcd.h" 

#define STATE_INIT        0
#define STATE_MODE_SELECT 1
#define STATE_COUNTDOWN   2
#define STATE_WAIT        3
#define STATE_MEASURE     4
#define STATE_DONE        5

volatile unsigned long time_ticks = 0;    
volatile unsigned int is_running = 0;     
volatile unsigned int update_display = 1;
volatile unsigned int button_pressed = 0; 
volatile unsigned int toggle_pressed = 0; 
volatile unsigned int restart_pressed = 0;

unsigned int current_state = 0;
unsigned long reaction_times[10];         
unsigned int test_count = 0;

// Go/No-Go and Penalties variables
unsigned int is_go_trial = 1; 
unsigned long penalty_ticks = 0; 
unsigned long avg = 0;
unsigned long sum = 0;

// Game Mode Variable
unsigned int game_mode = 0; // 0 = Go/No-Go, 1 = Internal Timer

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

    // BUTTONS (P2.2 is Toggle, P2.3 is Select)
    P2DIR &= ~(BIT2 | BIT3);                
    P2OUT |= (BIT2 | BIT3);                 
    P2REN |= (BIT2 | BIT3);                 
    P2IE  |= (BIT2 | BIT3);                 
    P2IES |= (BIT2 | BIT3);                
    P2IFG &= ~(BIT2 | BIT3);                

    // STOPWATCH TIMER SETUP (Timer 1)
    TA1CCR0 = 1250 - 1;             
    TA1CCTL0 = CCIE;                
    TA1CTL = TASSEL_2 | ID_3 | MC_1 | TACLR; 

    _BIS_SR(GIE);             

    // MAIN LOOP
    while(1) {
        if (current_state == STATE_INIT) {
            P2IE &= ~(BIT2 | BIT3); 
            clearLCD();
            delay_ms(5);
            
            writeCommand(0x80); 
            printString(" Hit button to  ");
            writeCommand(0xC0); 
            printString("     start      ");
            
            P2IFG &= ~(BIT2 | BIT3); 
            P2IE |= (BIT2 | BIT3);   
            
            button_pressed = 0; 
            while(!button_pressed); 
            current_state = STATE_MODE_SELECT;
        }
        
        // MODE SELECTION
        else if (current_state == STATE_MODE_SELECT) {
            P2IE &= ~(BIT2 | BIT3);
            clearLCD();
            delay_ms(5);
            
            writeCommand(0x80);
            printString(" Mode: Go/No-Go ");
            writeCommand(0xC0);
            printString("P2.2:Tgl P2.3:Ok");
            
            P2IFG &= ~(BIT2 | BIT3);
            P2IE |= (BIT2 | BIT3);
            
            button_pressed = 0;
            toggle_pressed = 0;
            game_mode = 0; // Default to Go/No-Go
            
            while(!button_pressed) {
                if (toggle_pressed) {
                    game_mode ^= 1; // Toggle between 0 and 1
                    
                    P2IE &= ~(BIT2 | BIT3);
                    writeCommand(0x80);
                    if (game_mode == 0) {
                        printString(" Mode: Go/No-Go ");
                    } else {
                        printString(" Mode: IntTimer ");
                    }
                    
                    toggle_pressed = 0;
                    P2IFG &= ~(BIT2 | BIT3);
                    P2IE |= (BIT2 | BIT3);
                }
            }
            current_state = STATE_COUNTDOWN;
        }
        
        else if (current_state == STATE_COUNTDOWN) {
            P2IE &= ~(BIT2 | BIT3); 
            clearLCD();
            delay_ms(5);
            
            writeCommand(0x80); 
            printString(" Wait for green "); 
            
            writeCommand(0xC0);
            if (game_mode == 0) {
                printString(" DO NOT hit red "); 
            } else {
                printString(" Wait exact 2.5s"); 
            }
            
            delay_ms(3000); 
            clearLCD(); printString("3"); delay_ms(1000); 
            clearLCD(); printString("2"); delay_ms(1000); 
            clearLCD(); printString("1"); delay_ms(1000); 
            clearLCD(); printString("GO!"); delay_ms(500); 
            clearLCD();
            
            P2IFG &= ~(BIT2 | BIT3); 
            P2IE |= (BIT2 | BIT3); 
            
            test_count = 0; 
            penalty_ticks = 0; 
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
            
            if (game_mode == 0) {
                is_go_trial = (TA1R % 4) != 0; // 75% Green, 25% Red
            } else {
                is_go_trial = 1; // Internal Timer LED is Green
            }

            button_pressed = 0; 
            current_state = STATE_MEASURE;
        }
        
        else if (current_state == STATE_MEASURE) {
            time_ticks = 0;     
            is_running = 1;     
            
            if (is_go_trial) {
                // GO TRIAL (GREEN LED)
                P2OUT |= BIT5; 
                
                // Stopwatch for Internal Timer Mode
                if (game_mode == 1) {
                    P2IE &= ~(BIT2 | BIT3);
                    clearLCD(); delay_ms(5);
                    writeCommand(0x80); 
                    printString("  Target: 2.5s  "); 
                    writeCommand(0xC0); 
                    printString("   Timing...    "); 
                    P2IFG &= ~(BIT2 | BIT3);
                    P2IE |= (BIT2 | BIT3);
                }
                
                while(!button_pressed) { 
                    if (update_display == 1) {
                        // Live stopwatch for Go/No-Go 
                        if (game_mode == 0) {
                            printTime(time_ticks);
                        }
                        update_display = 0;
                    }
                }
                
                is_running = 0;     
                P2OUT &= ~BIT5;     
                
                reaction_times[test_count] = time_ticks; 
                
                if (game_mode == 1) {
                    P2IE &= ~(BIT2 | BIT3);
                    clearLCD(); delay_ms(5);
                    printTime(time_ticks); 
                    delay_ms(1500); 
                    P2IFG &= ~(BIT2 | BIT3);
                    P2IE |= (BIT2 | BIT3);
                }
                
                test_count++;
            } 
            else {
                // NO-GO TRIAL (RED LED)
                P2OUT |= BIT4; 
                
                P2IE &= ~(BIT2 | BIT3); 
                clearLCD(); delay_ms(5);
                writeCommand(0x80);
                printString("    HOLD...     "); 
                
                P2IFG &= ~(BIT2 | BIT3);
                P2IE |= (BIT2 | BIT3);  
                
                while(time_ticks < 200 && !button_pressed);
                
                is_running = 0;
                P2OUT &= ~BIT4; 

                P2IE &= ~(BIT2 | BIT3); 
                clearLCD(); delay_ms(5);
                
                if (button_pressed) {
                    writeCommand(0x80); printString("FAILED! Penalty:");
                    writeCommand(0xC0); printString("     +500ms     ");
                    penalty_ticks += 50; 
                } else {
                    writeCommand(0x80); printString(" SUCCESS! Good  "); 
                    writeCommand(0xC0); printString("   Discipline   "); 
                }
                
                delay_ms(2000);
                clearLCD();
                P2IFG &= ~(BIT2 | BIT3);
                P2IE |= (BIT2 | BIT3);  
            }
            
            if (test_count < 10) {
                current_state = STATE_WAIT; 
            } else {
                current_state = STATE_DONE; 
            }
        }
        
        else if (current_state == STATE_DONE) {
            int i; 
            P2IE &= ~(BIT2 | BIT3); 

            // Blink LEDs
            for(i = 0; i < 10; i++) {
                P2OUT ^= (BIT4 | BIT5); 
                delay_ms(200); 
            }
            P2OUT &= ~(BIT4 | BIT5);  
            
            initLCD();
            delay_ms(5);
            
            // CALCULATE FINAL SCORES BASED ON MODE
            if (game_mode == 0) {
                // Go/No-Go: Calculate Average Time
                sum = 0;
                for(i = 0; i < 10; i++) {
                    sum += reaction_times[i];
                }
                avg = (sum + penalty_ticks) / 10;
                printAvg(avg); 
            } 
            else {
                // Internal Human Timer: Calculate Average Error Percentage
                unsigned long total_error = 0;
                for(i = 0; i < 10; i++) {
                    // Target is exactly 250 ticks (2.5 seconds)
                    unsigned long err;
                    if (reaction_times[i] > 250) {
                        err = reaction_times[i] - 250;
                    } else {
                        err = 250 - reaction_times[i];
                    }
                    total_error += err;
                }
                unsigned long avg_err_pct = (total_error * 100) / (10 * 250); 
                
                if (avg_err_pct > 999) avg_err_pct = 999; 
                
                printError(avg_err_pct);
            }
            
            delay_ms(4000); 
            
            clearLCD(); delay_ms(5); 
            writeCommand(0x80);
            printString("   Game Over!   "); 
            writeCommand(0xC0);
            printString("Both Btns -> Rst"); 
            
            P2IFG &= ~(BIT2 | BIT3); 
            P2IE |= (BIT2 | BIT3);   
            restart_pressed = 0;
            
            while (!restart_pressed);
            
            TA1CTL = TASSEL_2 | ID_3 | MC_1 | TACLR; 
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
    for(delay=0 ; delay<12345 ; delay=delay+1); // Debounce
    
    // Check BOTH P2.2 and P2.3 (Restart button)
    if ((P2IN & BIT2) == 0 && (P2IN & BIT3) == 0) {
      restart_pressed = 1;
      P2IFG &= ~(BIT2 | BIT3);
    }
    // Check P2.3 (Select/Action Button)
    else if (P2IFG & BIT3) { 
      button_pressed = 1;  
      P2IFG &= ~BIT3;      
    }
    // Check P2.2 (Toggle Button)
    else if (P2IFG & BIT2) {
      toggle_pressed = 1;
      P2IFG &= ~BIT2;
    }
}
