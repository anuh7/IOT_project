/* @file      irq.c
 * @version   1.0
 * @brief     LETIMER0 Interrupt routine file.
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

#include <stdint.h>

#include "app.h"
#include "em_i2c.h"
#include "em_gpio.h"
#include "sl_i2cspm.h"
#include "em_letimer.h"
#include "src/irq.h"
#include "gpio.h"
#include "src/scheduler.h"
#include "src/timers.h"


#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"


uint32_t count = 0;

void LETIMER0_IRQHandler (void)
{
  uint32_t flags;

  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();

  flags = LETIMER_IntGetEnabled(LETIMER0);

  LETIMER_IntClear(LETIMER0, flags);

  CORE_EXIT_CRITICAL();

  if (flags & LETIMER_IF_UF)                    // checking the underflow flag bit
    {
      schedulerSetEventUF();
      count++;
    }

  if (flags & LETIMER_IF_COMP1)                 // checking the COMP1 flag bit - DOS
  {
      gpioLed0SetOff();
      gpioLed1SetOff();
      gpioPD10Off();
      // DOS: Probably a bug, you want to disable this IRQ until the next call to timerWaitUs_interrupt()
      //      otherwise it will keep going off.
      LETIMER_IntDisable(LETIMER0, LETIMER_IEN_COMP1);        // disabling COMP1 interrupt
      schedulerSetEventCOMP1();                               // setting COMP1 event
  }

}

void I2C0_IRQHandler(void)
{
  I2C_TransferReturn_TypeDef transferStatus;
  transferStatus = I2C_Transfer(I2C0);             // continue an initiated I2C transfer in interrupt mode
  if (transferStatus == i2cTransferDone)          // I2C transfer completed successfully
    schedulerSetEventI2CTransfer();                // setting I2C transaction event

  if (transferStatus < 0)
      LOG_ERROR("%d", transferStatus);              // error logging
}

uint32_t letimerMilliseconds(void)
{
    uint32_t time;
    time = (count)*3000;
    return time;
}
