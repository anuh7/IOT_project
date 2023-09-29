/* @file      scheduler.c
 * @version   1.0
 * @brief     Functions to maintain scheduling of events
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

#include "src/scheduler.h"
#include "src/i2c.h"
#include "src/gpio.h"
#include "src/timers.h"

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"


#define POWERUP_TIME      (80000)
#define CONVERTION_TIME    (10800)

enum {
  evtUF_LETIMER0 = 0x01,           // event for underflow interrupt
  evtCOMP1_LETIMER0 = 0x02,        // event for COMP1 interrupt
  evt_I2CTransferComplete = 0x04    // event for I2C transaction
};

#define   MY_NUM_STATES (5)
typedef enum uint32_t {
  STATE0_IDLE,                      // state for POR sequence
  STATE1_TIMER_WAIT,                // state for writing I2C command
  STATE2_WARMUP,                    // state for conversion time delay
  STATE3_MEASUREMENT,               // state for reading I2C command
  STATE4_REPORT,                    // state to print temperature values
}my_states;

uint32_t myEvents = 0;          // variable to hold all events

void schedulerSetEventUF()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();            // enter critical, turn off interrupts in NVIC
  myEvents |= evtUF_LETIMER0;       // RMW
  CORE_EXIT_CRITICAL();             // exit critical, re-enable interrupts in NVIC
}

void schedulerSetEventCOMP1()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();            // enter critical, turn off interrupts in NVIC
  myEvents |= evtCOMP1_LETIMER0;    // RMW
  CORE_EXIT_CRITICAL();             // exit critical, re-enable interrupts in NVIC
}

void schedulerSetEventI2CTransfer()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();                    // enter critical, turn off interrupts in NVIC
  myEvents |= evt_I2CTransferComplete;      // RMW
  CORE_EXIT_CRITICAL();                     // exit critical, re-enable interrupts in NVIC
}

uint32_t getNextEvent()
{
  static uint32_t theEvent;                 // select 1 event to return to main() code

  if (myEvents & evtUF_LETIMER0)
    {
      theEvent = evtUF_LETIMER0;

      CORE_DECLARE_IRQ_STATE;            // enter critical section
      CORE_ENTER_CRITICAL();

      myEvents &= ~(evtUF_LETIMER0);        // clearing the event in the data structure

      CORE_EXIT_CRITICAL();
    }
  else if (myEvents & evtCOMP1_LETIMER0)
    {
      theEvent = evtCOMP1_LETIMER0;

      CORE_DECLARE_IRQ_STATE;            // enter critical section
      CORE_ENTER_CRITICAL();

      myEvents &= ~(evtCOMP1_LETIMER0);

      CORE_EXIT_CRITICAL();
    }
  else if (myEvents & evt_I2CTransferComplete)
    {
      theEvent = evt_I2CTransferComplete;

      CORE_DECLARE_IRQ_STATE;            // enter critical section
      CORE_ENTER_CRITICAL();

      myEvents &= ~(evt_I2CTransferComplete);

      CORE_EXIT_CRITICAL();
    }
  // exit critical section

  return (theEvent);
}


void state_machine(uint32_t event)
{
  static my_states nextState = STATE0_IDLE;

  switch (nextState)
  {
    case STATE0_IDLE:
      nextState = STATE0_IDLE;      //default
      if (event & evtUF_LETIMER0)
        {
          sensor_enable();                          // power up the device
          timerWaitUs_interrupt(POWERUP_TIME);      // interrupt for powerup time
          nextState = STATE1_TIMER_WAIT;
        }
      break;

    case STATE1_TIMER_WAIT:
      nextState = STATE1_TIMER_WAIT;      //default
      if (event & evtCOMP1_LETIMER0)
        {
          initI2C();                                 // initialise I2C
          sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);    // Power requirement for I2C
          i2c_write();                               // I2C write command
          nextState = STATE2_WARMUP;
        }
      break;

    case STATE2_WARMUP:
      nextState = STATE2_WARMUP;      //default
      if (event & evt_I2CTransferComplete)
        {
          sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
          timerWaitUs_interrupt(CONVERTION_TIME);
          nextState = STATE3_MEASUREMENT;
        }
      break;

    case STATE3_MEASUREMENT:
      nextState = STATE3_MEASUREMENT;      //default
      if (event & evtCOMP1_LETIMER0)
        {
          sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
          i2c_read();
          nextState = STATE4_REPORT;
        }
      break;

    case STATE4_REPORT:
      nextState = STATE4_REPORT;      //default
      if (event & evt_I2CTransferComplete)
        {

          sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
          sensor_disable();
          NVIC_DisableIRQ(I2C0_IRQn);
          read_temp_from_si7021();
          nextState = STATE0_IDLE;
        }
      break;
  }

}
