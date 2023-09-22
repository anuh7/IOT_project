/* @file      irq.c
 * @version   1.0
 * @brief     LETIMER0 Interrupt routine file.
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

#include <stdint.h>

#include "em_letimer.h"
#include "src/irq.h"
#include "gpio.h"

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"
#include "src/scheduler.h"


void LETIMER0_IRQHandler (void)
{
  uint32_t flags;

  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();

  flags = LETIMER_IntGetEnabled(LETIMER0);

  LETIMER_IntClear(LETIMER0, flags);
  schedulerSetEventUF();

  CORE_EXIT_CRITICAL();

//  if (flags & LETIMER_IF_UF)
//    {
//      gpioLed0SetOn();
//    }
//
//  if (flags & LETIMER_IF_COMP1)
//  {
//     gpioLed0SetOff();
//  }
}
