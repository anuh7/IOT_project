/* @file      scheduler.h
 * @version   1.0
 * @brief     Application interface provided for scheduler.c
 *
 * @author    Anuhya Kuraparthy, anuhya.kuraparthy@colorado.edu
 * @date      Oct 06, 2023
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823: IoT Embedded Firmware
 * @instructor  David Sluiter
 *
 * @assignment Assignment 7 - Bluetooth BLE Client
 * @due        Oct 20
 *
 * @resources  -
 */


#ifndef SRC_SCHEDULER_H_
#define SRC_SCHEDULER_H_

#include <stdint.h>

#include "sl_bluetooth.h"

enum {
  evtUF_LETIMER0 = 0x01,           // event for underflow interrupt
  evtCOMP1_LETIMER0 = 0x02,        // event for COMP1 interrupt
  evt_I2CTransferComplete = 0x04,    // event for I2C transaction
  evt_button_released = 0x08,
  evt_button_pressed = 0x16
};



/**
* @brief Scheduler routine to return 1 event to main()code and clear that event
*
*
* @param void
* @return uint32_t  Returns the event set in the scheduler
*/
//uint32_t getNextEvent();


/**
* @brief Function to set the LE timer underflow interrupt in the event structure
*
*
* @param void
* @return void
*/
void schedulerSetEventUF();

/**
* @brief Function to set the LE timer COMP1 interrupt in the event structure
*
*
* @param void
* @return void
*/
void schedulerSetEventCOMP1();

/**
* @brief Function to set the completion of i2c transaction in the event structure
*
*
* @param void
* @return void
*/
void schedulerSetEventI2CTransfer();

void schedulerSetEventButtonReleased();
void schedulerSetEventButtonPressed();

/**
* @brief State machine to control order of events
*
*
* @param uint32_t event occurred is passed to decide on state transition
* @return void
*/
void state_machine(sl_bt_msg_t *evt);

/**
* @brief State machine to control order of events for discovering a server device
*
*
* @param uint32_t event occurred is passed to decide on state transition
* @return void
*/
void discovery_state_machine(sl_bt_msg_t *evt);



#endif /* SRC_SCHEDULER_H_ */
