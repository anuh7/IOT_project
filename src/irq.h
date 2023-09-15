/* @file      irq.h
 * @version   1.0
 * @brief     Application interface provided for irq.c
 *
 * @author    Anuhya Kuraparthy, anuhya.kuraparthy@colorado.edu
 * @date      Sept 15, 2021
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823: IoT Embedded Firmware
 * @instructor  David Sluiter
 *
 * @assignment Assignment 2- Managing Energy modes
 * @due        Sept 15
 *
 * @resources  -
 */


#ifndef SRC_IRQ_H_
#define SRC_IRQ_H_


/**
* @brief Interrupt handler for the LETIMER0
*
* The Low Energy Timer COMP0 and UF flags are checked to see which one is
* set and the corresponding actions is performed.
*
* @param void
* @return void
*/
void LETIMER0_IRQHandler (void);

#endif /* SRC_IRQ_H_ */
