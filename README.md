# MSP430 Human Reaction Timer

A microcontroller-based embedded system that tests and measures human reaction time. Built using the Texas Instruments MSP430G2553 Launchpad, this project runs a 10-trial sequence, dynamically randomizes the wait time between trials, and calculates the user's average reaction time in milliseconds.

## Features
* **10-Trial Sequence:** Automatically loops through 10 consecutive reaction tests.
* **Dynamic Wait Times:** Prevents the user from predicting the light by calculating a dynamic delay `(test_count * previous_time) + 4000ms` before the next stimulus.
* **Live LCD Feedback:** Displays countdowns, current trial times, and the final calculated average using a 16x2 character LCD.
* **Hardware Interrupts:** Utilizes Timer1 for precise 10ms time tracking and Port 2 hardware interrupts for instant button-press detection.
* **State Machine Architecture:** Clean, non-blocking `while(1)` loop separated into discrete states (Init, Countdown, Wait, Measure, Done).

## Hardware Requirements
* **Microcontroller:** TI MSP430G2553 Launchpad
* **Display:** 16x2 Character LCD (8-bit mode)
* **Inputs:** 1x Tactile Push Button
* **Outputs:** 2x LEDs (e.g., 1 Green "Go" LED, 1 additional LED for end-sequence blinking)
* Breadboard, jumper wires, and appropriate current-limiting resistors for the LEDs.

## Pinout & Wiring Guide

**⚠️ IMPORTANT: Hardware Conflict Avoidance**
Because the LCD requires 8 data lines, all of Port 1 (`P1.0` - `P1.7`) is dedicated to the LCD. Do not use the Launchpad's onboard LEDs (which are hardwired to P1.0 and P1.6), as this will cause the LCD to crash or display garbage text. 

### LCD Connections
| LCD Pin | MSP430 Pin | Function |
| :--- | :--- | :--- |
| RS | `P2.0` | Register Select |
| E | `P2.1` | Enable Pulse |
| D0 - D7 | `P1.0` - `P1.7` | 8-bit Data Bus |

### External Components
| Component | MSP430 Pin | Wiring Instructions |
| :--- | :--- | :--- |
| **Tactile Button** | `P2.3` | Connect one leg to `P2.3`, and the diagonally opposite leg to `GND`. (Internal pull-up resistor is enabled in code). |
| **LED 1 (Toggle)** | `P2.4` | Connect `P2.4` -> Resistor -> LED Anode -> LED Cathode -> `GND`. |
| **LED 2 (Green/Go)** | `P2.5` | Connect `P2.5` -> Resistor -> LED Anode -> LED Cathode -> `GND`. |

## How It Works (The State Machine)
The core of the program operates on a 5-stage State Machine:
1. **STATE_INIT:** Waits for the user to press the button to begin the game.
2. **STATE_COUNTDOWN:** Displays a 3-2-1-GO! countdown on the LCD.
3. **STATE_WAIT:** Calculates and executes a dynamic, unpredictable delay before the green LED turns on.
4. **STATE_MEASURE:** Turns on the Green LED (`P2.5`), starts the stopwatch, and waits for the hardware interrupt from the button press. Saves the time to an array.
5. **STATE_DONE:** Blinks both LEDs, calculates the average of the 10 trials, and displays the final score on the LCD.

## Project Structure
* `main.c`: Contains the state machine logic, hardware initialization, and interrupt service routines (ISRs).
* `LCD.h`: Header file containing pin definitions and function prototypes for the LCD.
* `LCD.c`: Library handling the low-level 8-bit parallel communication, cursor management, and string/time formatting for the 16x2 screen.

## Setup and Execution
1. Open Code Composer Studio (CCS).
2. Create a new CCS Project for the `MSP430G2553`.
3. Add `main.c`, `LCD.c`, and `LCD.h` to your project folder.
4. Wire your breadboard according to the Pinout guide above.
5. Build the project (ensure 0 errors) and hit **Debug** to flash the code to your Launchpad.
6. Press the physical button connected to `P2.3` to start the test!
