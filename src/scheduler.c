/* @file      scheduler.c
 * @version   1.0
 * @brief     Functions to maintain scheduling of events
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

#include "src/scheduler.h"

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

enum {
  evtUF_LETIMER0 = 1,           // event for underflow interrupt
};


uint32_t myEvents = 0;          // variable to hold all events

void schedulerSetEventUF()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();            // enter critical, turn off interrupts in NVIC
  myEvents |= evtUF_LETIMER0;       // RMW
  CORE_EXIT_CRITICAL();             // exit critical, re-enable interrupts in NVIC
}



uint32_t getNextEvent()
{
    uint32_t theEvent;                 // select 1 event to return to main() code, apply priorities etc.

    theEvent = myEvents;               // 1 event to return to the caller

    CORE_DECLARE_IRQ_STATE;            // enter critical section
    CORE_ENTER_CRITICAL();

    myEvents = 0;                      // clear the event in your data structure (RMW)
    CORE_EXIT_CRITICAL();              // exit critical section

    return (theEvent);
}
