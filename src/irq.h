/* @file      irq.h
 * @version   1.0
 * @brief     Application interface provided for irq.c
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


#ifndef SRC_IRQ_H_
#define SRC_IRQ_H_

#include <stdint.h>

/**
* @brief Interrupt handler for the LETIMER0
*
* The Low Energy Timer UF flags are checked to see which one is
* set and the corresponding event is set.
*
* @param void
* @return void
*/
void LETIMER0_IRQHandler (void);

/**
* @brief Interrupt handler for the I2C. The function continues the transaction
*         initiated earlier by the peripheral.
*
*
* @param void
* @return void
*/
void I2C0_IRQHandler(void);

uint32_t letimerMilliseconds(void);
#endif /* SRC_IRQ_H_ */
