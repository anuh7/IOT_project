/* @file      scheduler.h
 * @version   1.0
 * @brief     Application interface provided for scheduler.c
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


#ifndef SRC_SCHEDULER_H_
#define SRC_SCHEDULER_H_

#include <stdint.h>


/**
* @brief Scheduler routine to return 1 event to main()code and clear that event
*
*
* @param void
* @return uint32_t  Returns the event set in the scheduler
*/
uint32_t getNextEvent();


/**
* @brief Function to set the LE timer underflow interrupt in the event structure
*
*
* @param void
* @return void
*/
void schedulerSetEventUF();


#endif /* SRC_SCHEDULER_H_ */
