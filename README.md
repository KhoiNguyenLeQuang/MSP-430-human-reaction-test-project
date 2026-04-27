# MSP430 Human Factors Testing Platform

A microcontroller-based embedded system that tests and measures human reaction time and internal time perception. Built using the Texas Instruments MSP430G2553 Launchpad, this project features two distinct testing modes, runs 10-trial sequences with dynamic randomization, and calculates final performance metrics on a 16x2 LCD.

## Features
* **Dual Game Modes:** * **Go/No-Go:** Tests visual reaction time and impulse control. Users must press the button as fast as possible on a Green light, but withhold their press on a Red light. Includes a 500ms penalty for failures.
  * **Internal Human Timer (HIT):** Tests human time perception. Users are challenged to press the button exactly 2.5 seconds after the Green light illuminates, with the live timer hidden from view.
* **4-Bit LCD Integration:** Efficiently drives a 16x2 character display using only 6 GPIO pins to provide live feedback, mode selection menus, and final calculated scores (Average Time or Average Error %).
* **Dynamic Wait Times:** Prevents the user from predicting the stimulus by calculating a dynamic delay `(test_count * previous_time) + 2000ms` before the next LED activates.
* **Hardware Interrupts & Timers:** Utilizes Timer1 for precise 10ms time tracking and Port 2 hardware interrupts for instant, non-blocking button-press detection.
* **State Machine Architecture:** Clean, robust `while(1)` loop separated into discrete states (Init, Mode Select, Countdown, Wait, Measure, Done).

## Hardware Requirements
* **Microcontroller:** TI MSP430G2553 Launchpad
* **Display:** 16x2 Character LCD (configured for 4-bit mode)
* **Inputs:** 2x Tactile Push Buttons
* **Outputs:** 2x LEDs (1 Red "No-Go", 1 Green "Go")
* Breadboard, jumper wires, a 10k potentiometer (for LCD contrast), and appropriate current-limiting resistors for the LEDs.

## Pinout & Wiring Guide

### LCD Connections (4-Bit Mode)
| LCD Pin | MSP430 Pin | Function |
| :--- | :--- | :--- |
| RS | `P2.0` | Register Select |
| E | `P2.1` | Enable Pulse |
| D4 | `P1.4` | Data Bus Bit 4 |
| D5 | `P1.5` | Data Bus Bit 5 |
| D6 | `P1.6` | Data Bus Bit 6 |
| D7 | `P1.7` | Data Bus Bit 7 |

*(Note: LCD pins D0-D3 are left disconnected in 4-bit mode. VSS, VDD, V0, A, and K must be wired to power, ground, and the potentiometer as standard).*

### External Components
| Component | MSP430 Pin | Wiring Instructions |
| :--- | :--- | :--- |
| **Toggle Button** | `P2.2` | Connect one leg to `P2.2`, and the diagonally opposite leg to `GND`. Used to toggle menus and restart the HIT mode. |
| **Select Button** | `P2.3` | Connect one leg to `P2.3`, and the diagonally opposite leg to `GND`. Used to lock in choices and act as the main reaction trigger. |
| **Red LED (No-Go)** | `P2.4` | Connect `P2.4` -> Resistor -> LED Anode -> LED Cathode -> `GND`. |
| **Green LED (Go)** | `P2.5` | Connect `P2.5` -> Resistor -> LED Anode -> LED Cathode -> `GND`. |

*(Note: Internal pull-up resistors are enabled in the software for P2.2 and P2.3).*

## How It Works (The State Machine)
The core of the program operates on a 6-stage State Machine:
1. **STATE_INIT:** Boot screen waiting for initial user interaction.
2. **STATE_MODE_SELECT:** Allows the user to toggle between "Go/No-Go" and "IntTimer" using `P2.2`, and confirm their choice with `P2.3`.
3. **STATE_COUNTDOWN:** Displays mode-specific instructions and a 3-2-1-GO! countdown sequence.
4. **STATE_WAIT:** Calculates and executes an unpredictable delay. Determines if the upcoming trial will be a Go (Green) or No-Go (Red) stimulus based on pseudo-random timer data (HIT mode is always Green).
5. **STATE_MEASURE:** Activates the target LED. 
   * In *Go/No-Go*, displays a live stopwatch and waits for a `P2.3` press (or penalizes a press on Red).
   * In *HIT*, hides the timer, waits for a `P2.3` press, and reveals the locked-in time for 1.5 seconds.
6. **STATE_DONE:** Blinks both LEDs, calculates the final metrics (Average Time for Go/No-Go; Average Error % for HIT), and displays the results. Requires a `P2.3` press to restart Go/No-Go, or a simultaneous `P2.2` + `P2.3` press to restart the HIT mode.

## Project Structure
* `main.c`: Contains the state machine logic, game mode mathematics, hardware initialization, and interrupt service routines (ISRs).
* `LCD.h`: Header file containing pin definitions, macros, and function prototypes for the LCD.
* `LCD.c`: Library handling the low-level 4-bit parallel communication, cursor management, custom time formatting, and error percentage string generation.

## Setup and Execution
1. Open Code Composer Studio (CCS).
2. Create a new CCS Project for the `MSP430G2553`.
3. Add `main.c`, `LCD.c`, and `LCD.h` to your project directory.
4. Wire your hardware according to the Pinout guide above (ensure the LCD is wired for 4-bit operation).
5. Build the project (ensure 0 errors) and hit **Debug** to flash the firmware to your Launchpad.
6. Press the physical Select button (`P2.3`) to begin testing your human factors!`P2.3` to start the test!
