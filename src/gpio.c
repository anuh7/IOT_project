/*
  gpio.c
 
   Created on: Dec 12, 2018
       Author: Dan Walkes
   Updated by Dave Sluiter Dec 31, 2020. Minor edits with #defines.

   March 17
   Dave Sluiter: Use this file to define functions that set up or control GPIOs.
   
   Jan 24, 2023
   Dave Sluiter: Cleaned up gpioInit() to make it less confusing for students regarding
                 drive strength setting. 

 *
 * @student    Anuhya Kuraparthy, anuhya.kuraparthy@colorado.edu
 *
 
 */


// *****************************************************************************
// Students:
// We will be creating additional functions that configure and manipulate GPIOs.
// For any new GPIO function you create, place that function in this file.
// *****************************************************************************

#include <stdbool.h>
#include "em_gpio.h"
#include <string.h>

#include "gpio.h"

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

// Student Edit: Define these, 0's are placeholder values.
//
// See the radio board user guide at https://www.silabs.com/documents/login/user-guides/ug279-brd4104a-user-guide.pdf
// and GPIO documentation at https://siliconlabs.github.io/Gecko_SDK_Doc/efm32g/html/group__GPIO.html
// to determine the correct values for these.
// If these links have gone bad, consult the reference manual and/or the datasheet for the MCU.
// Change to correct port and pins:
#define LED_port   (5)
#define LED0_pin   (4)
#define LED1_pin   (5)
#define sensor_port  (3)
#define sensor_pin   (15)
#define LCD_port   (3)
#define LCD_pin     (13)



// Set GPIO drive strengths and modes of operation
void gpioInit()
{
    // Set the port's drive strength. In this MCU implementation, all GPIO cells
    // in a "Port" share the same drive strength setting. 
	//GPIO_DriveStrengthSet(LED_port, gpioDriveStrengthStrongAlternateStrong); // Strong, 10mA
	GPIO_DriveStrengthSet(LED_port, gpioDriveStrengthWeakAlternateWeak); // Weak, 1mA
	
	// Set the 2 GPIOs mode of operation
	GPIO_PinModeSet(LED_port, LED0_pin, gpioModePushPull, false);
	GPIO_PinModeSet(LED_port, LED1_pin, gpioModePushPull, false);

  GPIO_DriveStrengthSet(sensor_port, gpioDriveStrengthWeakAlternateWeak); // Weak, 1mA
  GPIO_PinModeSet(sensor_port, sensor_pin, gpioModePushPull, false);


  // DOS: Test for GPIO used for ADC0 input, testing to see if I can config it for push-pull output
  // PD10 traces to Expansion header 7. Plan is to hook up the logic analyzer to this pin
  // to test timerWaitUs_interrupt()
  GPIO_DriveStrengthSet(PD_port, gpioDriveStrengthWeakAlternateWeak);       /* Attribute: Instructor Dave Sluiter*/
  GPIO_PinModeSet(PD_port, PD_port10, gpioModePushPull, false);

  gpioPD10Off();

  /* Set the pin of Push Button 0 as input with pull-up resistor */
  //(0 = down and 1 = up).      AN0012

  GPIO_PinModeSet(button_port, button_pin, gpioModeInputPullFilter, true);
  GPIO_ExtIntConfig(button_port, button_pin, button_pin, true, true, true);

} // gpioInit()


void gpioLed0SetOn()
{
	GPIO_PinOutSet(LED_port, LED0_pin);
}


void gpioLed0SetOff()
{
	GPIO_PinOutClear(LED_port, LED0_pin);
}


void gpioLed1SetOn()
{
	GPIO_PinOutSet(LED_port, LED1_pin);
}


void gpioLed1SetOff()
{
	GPIO_PinOutClear(LED_port, LED1_pin);
}


void sensor_enable()
{
  GPIO_PinOutSet(sensor_port, sensor_pin);
}


void sensor_disable()
{
  GPIO_PinOutClear(sensor_port, sensor_pin);
}


// DOS: test code
void gpioPD10On()     /* Attribute: Instructor Dave Sluiter*/
{
  GPIO_PinOutSet(PD_port,PD_port10); // routes to Expansion Header pin 7
}


void gpioPD10Off()
{
  GPIO_PinOutClear(PD_port,PD_port10); // routes to Expansion Header pin 7
}

void gpioSetDisplayExtcomin(bool value)
{
    if(value)
      {
        GPIO_PinOutSet(LCD_port,LCD_pin);
      }
    else
      {
        GPIO_PinOutClear(LCD_port,LCD_pin);
      }
}
