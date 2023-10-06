
/* @file      timers.h
 * @version   1.0
 * @brief     Application interface provided for timers.c
 *
 * @author    Anuhya Kuraparthy, anuhya.kuraparthy@colorado.edu
 * @date      Sept 29, 2023
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823: IoT Embedded Firmware
 * @instructor  David Sluiter
 *
 * @assignment Assignment 4- Si7021 and Load Power Management
 * @due        Sept 29
 *
 * @resources  -
 */

#ifndef SRC_TIMERS_H_
#define SRC_TIMERS_H_

/**
* @brief Function to initialize the Low energy timer peripheral.
*
* Depending on the low energy mode selected in app.h, oscillator is selected
* accordingly and its prescalar is set.
*
* @param void
* @return void
*/
void initLETIMER0();

/**
* @brief Function to create delay using LE timer and generate COMP1 interrupt
*
*
* @param us_wait  delay required in microseconds
* @return void
*/
void timerWaitUs_interrupt(uint32_t us_wait);

/**
* @brief Function to create delay
*
*
* @param us_wait  delay required in microseconds
* @return void
*/
void timerWaitUs_polled(uint32_t us_wait);

#endif /* SRC_TIMERS_H_ */
