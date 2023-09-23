/* @file      irq.h
 * @version   1.0
 * @brief     Application interface provided for irq.c
 *
 * @author    Anuhya Kuraparthy, anuhya.kuraparthy@colorado.edu
 * @date      Sept 22, 2021
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


#ifndef SRC_IRQ_H_
#define SRC_IRQ_H_


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

#endif /* SRC_IRQ_H_ */
