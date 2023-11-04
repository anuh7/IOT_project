/* @file      irq.c
 * @version   1.0
 * @brief     Interrupt routine file.
 *
 * @author    Anuhya Kuraparthy, anuhya.kuraparthy@colorado.edu
 * @date      Oct 27th, 2023
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823: IoT Embedded Firmware
 * @instructor  David Sluiter
 *
 * @assignment Assignment 8
 * @due        oct 27th
 *
 * @resources  https://github.com/ryankurte/silabs-rail/blob/master/submodules/emdrv/gpiointerrupt/src/gpiointerrupt.c
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
#include "ble.h"
#include "sl_bt_api.h"
#include "src/ble_device_type.h"
#include "sl_bluetooth.h"


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
      // DOS: Probably a bug, you want to disable this IRQ until the next call to timerWaitUs_interrupt()
      //      otherwise it will keep going off.
      LETIMER_IntDisable(LETIMER0, LETIMER_IEN_COMP1);        /* Attribution: Instructor Dave Sluiter*/
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

void GPIO_EVEN_IRQHandler(void)
{
  uint32_t iflags;

  iflags = GPIO_IntGetEnabled();
  GPIO_IntClear(iflags);
  unsigned int button_value = GPIO_PinInGet(button_port, button_pin);

  ble_data_struct_t *bleDataPtr = getBleDataPtr();
  if (button_value)       //pressed = 0
    {
      bleDataPtr->button_status = false;
      schedulerSetEventButtonReleased();
    }
  else
    {
      bleDataPtr->button_status = true;
      schedulerSetEventButtonPressed();
    }
}


#if !DEVICE_IS_BLE_SERVER

void GPIO_ODD_IRQHandler(void) {

  ble_data_struct_t *bleDataPtr = getBleDataPtr();

  uint32_t iflags;

  iflags = GPIO_IntGetEnabled();
  GPIO_IntClear(iflags);

  unsigned int button_value = GPIO_PinInGet(button_port_pb1, button_pin_pb1);

  if (button_value)       //pressed = 0
    {
      bleDataPtr->pb1_button_status = false;
      schedulerSetEventButtonReleased();
    }
  else
    {
      bleDataPtr->pb1_button_status = true;
      schedulerSetEventButtonPressed();
    }
}

#endif
