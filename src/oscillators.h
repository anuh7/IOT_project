/* @file      oscillators.h
 * @version   1.0
 * @brief     Application interface provided for oscillators.c
 *
 * @author    Anuhya Kuraparthy, anuhya.kuraparthy@colorado.edu
 * @date      Sept 22, 2023
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823: IoT Embedded Firmware
 * @instructor  David Sluiter
 *
 * @assignment Assignment 3- Si7021 and Load Power Management
 * @due        Sept 22
 *
 * @resources  -
 */

#ifndef SRC_OSCILLATORS_H_
#define SRC_OSCILLATORS_H_


/**
* @brief Function to initialize different oscillators for low energy power modes
*
* Depending on the low energy mode selected in app.h, oscillator is selected
* accordingly and its prescalar is set.
*
* @param void
* @return void
*/
void oscillator_init();

#endif /* SRC_OSCILLATORS_H_ */
