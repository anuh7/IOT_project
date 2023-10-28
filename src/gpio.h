/*
   gpio.h
  
    Created on: Dec 12, 2018
        Author: Dan Walkes

    Updated by Dave Sluiter Sept 7, 2020. moved #defines from .c to .h file.
    Updated by Dave Sluiter Dec 31, 2020. Minor edits with #defines.

    Editor: Feb 26, 2022, Dave Sluiter
    Change: Added comment about use of .h files.

 *
 * @student    Anuhya Kuraparthy, anuhya.kuraparthy@colorado.edu
 *
 */


// Students: Remember, a header file (a .h file) generally defines an interface
//           for functions defined within an implementation file (a .c file).
//           The .h file defines what a caller (a user) of a .c file requires.
//           At a minimum, the .h file should define the publicly callable
//           functions, i.e. define the function prototypes. #define and type
//           definitions can be added if the caller requires theses.


#ifndef SRC_GPIO_H_
#define SRC_GPIO_H_



// DOS: Test for GPIO used for ADC0 input, testing to see if I can config it for push-pull output
// PD10 traces to Expansion header 7
#define PD_port     gpioPortD
#define PD_port10   10
#define button_port   gpioPortF
#define button_pin   6




// Function prototypes
void gpioInit();
void gpioLed0SetOn();
void gpioLed0SetOff();
void gpioLed1SetOn();
void gpioLed1SetOff();
void sensor_enable();
void sensor_disable();



/* Attribution: Instructor Dave Sluiter*/
void gpioPD10On(void);
void gpioPD10Off(void);

void gpioSetDisplayExtcomin(bool value);

#endif /* SRC_GPIO_H_ */
