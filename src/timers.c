/* @file      timers.c
 * @version   1.0
 * @brief     Functions to initialise the low energy timer and create delay
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

#include "em_letimer.h"
#include "app.h"
#include "em_cmu.h"

#include "src/oscillators.h"
#include "src/timers.h"
#include "src/gpio.h"

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#define ACTUAL_CLK_FREQ       (32768/4)
//#define ACTUAL_CLK_FREQ CMU_ClockFreqGet(cmuClock_LETIMER0)                    // frequency of the selected oscillator
#define VALUE_TO_LOAD_COMP0 ((LETIMER_PERIOD_MS*ACTUAL_CLK_FREQ)/1000)        // 3000 ms
#define MIN_VALUE (1)


const LETIMER_Init_TypeDef LETIMER_INIT_VALUES =
  {
    false,              /* Do not Enable timer when initialization completes. */
//    false,             /* Stop counter during debug halt. */
    true,             /* Do not stop counter during debug halt. */
    true,             /* Do load COMP0 into CNT on underflow. */
    false,             /* Do not load COMP1 into COMP0 when REP0 reaches 0. */
    0,                 /* Idle value 0 for output 0. */
    0,                 /* Idle value 0 for output 1. */
    letimerUFOANone,   /* No action on underflow on output 0. */
    letimerUFOANone,   /* No action on underflow on output 1. */
    letimerRepeatFree, /* Count until stopped by SW. */
    0                  /* Comp0 top Value. */
  };

void initLETIMER0()
{
  LETIMER_Init(LETIMER0, &LETIMER_INIT_VALUES);         /* Initialise LE timer with the structure passed */

  LETIMER_CompareSet(LETIMER0, 0, VALUE_TO_LOAD_COMP0);   /* Load the timer period in COMP0 */

  LETIMER_IntClear (LETIMER0, 0xFFFFFFFF);

  // Set UF and COMP1 in LETIMER0_IEN, so that the timer will generate IRQs to the NVIC.
  // DOS: I think you want to only the UF IEN, and set the COMP1 IEN bit in timerWaitUs_interrupt()
//  uint32_t temp = LETIMER_IEN_COMP1 | LETIMER_IEN_UF;                  /* Attributions: Devang*/
  uint32_t temp = LETIMER_IEN_UF; // DOS Just UF

  // Enable LETIMER Interrupt
  LETIMER_IntEnable (LETIMER0, temp);

  LETIMER_Enable (LETIMER0, true);                  /* Start/Enable the timer */

  // DOS: Read it a few times to make sure it's running, step over these in the debugger,
  //      make sure you are getting the values you expect
  temp = LETIMER_CounterGet (LETIMER0);
  temp = LETIMER_CounterGet (LETIMER0);
  temp = LETIMER_CounterGet (LETIMER0);
  temp = LETIMER_CounterGet (LETIMER0);
  temp = LETIMER_CounterGet (LETIMER0);
  temp = LETIMER_CounterGet (LETIMER0);

}


void timerWaitUs_interrupt(uint32_t us_wait)
{
  uint16_t current_tick, target_comp0_value, wrap_amount;
  uint16_t time_in_us_per_tick;
  uint16_t ticks_required;

  uint32_t frequency = CMU_ClockFreqGet (cmuClock_LETIMER0); // 1000 for EM3, 8192 for EM0 to 2


  // compute actual time/tick
  if (frequency == 1000)
    time_in_us_per_tick = 1000; // 1000us = 1ms
  else
    time_in_us_per_tick = 122;  // 122us = 1/8192

//  uint32_t ticks_required = (us_wait/1000);
//  ticks_required = ((ticks_required*ACTUAL_CLK_FREQ)/(1000));          /* Number of ticks required*/

  // This is the number of counter changes we need to see in order for the requested amount of delay to pass
  ticks_required = us_wait / time_in_us_per_tick;

  //10800/1000=10.8
  //10.8*8192

  // range check
  if (ticks_required > VALUE_TO_LOAD_COMP0)         /* Clamping the delay to LE timer period*/
    {
      ticks_required = (VALUE_TO_LOAD_COMP0);
      LOG_ERROR("Delay requested is longer than time period; Clamping delay to LE timer time period \n\r");
    }
  else if (ticks_required < (MIN_VALUE))            /* Clamping the delay to LE timer resolution*/
    {
      ticks_required = (MIN_VALUE);
      LOG_ERROR("Delay requested is shorter than 1ms; Clamping delay to LE timer resolution \n\r");
    }

  //current=1000-4000
  //(0x5000)-(0x4000-0x1000)=0x2000.

      current_tick = LETIMER_CounterGet(LETIMER0);

      target_comp0_value = current_tick - ticks_required;

      // if target_comp0_value is > COMP0 register value, it means we wrapped around
      if (target_comp0_value > LETIMER0->COMP0) {
          wrap_amount = 0xFFFF - target_comp0_value + 1;
          //                   COMP0           - (amount we wrapped around by)
          target_comp0_value = LETIMER0->COMP0 - wrap_amount;
      }

      // When we get here target_comp0_value contains the COMP0 value we need to wait
      // for the counter to get to in order for the requested amount of delay to have passed.

      LOG_INFO("Cnt=%d, Tr=%d, Tc0=%d", current_tick, ticks_required, target_comp0_value);

//      if (current_tick >= ticks_required){
//      delay_tick = current_tick - ticks_required;           /* LE timer is a countdown timer*/
//      }
//
//      else{                    // overflow condition
//        delay_tick = (VALUE_TO_LOAD_COMP0 - (ticks_required - current_tick));
//      }

      LETIMER_CompareSet(LETIMER0, 1, target_comp0_value);     /* Loading the delay period in COMP1 register*/

      // DOS: Set COMP1 IEN bit, cleared in your LETIMER0_IRQHandler() if COMP1 IF bit is set.
      //      You have this commented out in irq.c: LETIMER_IntDisable(LETIMER0, LETIMER_IEN_COMP1);
      LETIMER_IntEnable (LETIMER0, LETIMER_IEN_COMP1);
      LETIMER0->IEN = LETIMER0->IEN | LETIMER_IEN_COMP1; // DOS fix for compiler bug


}





// -----------------------------------------------
// IRQ driven version - DOS: This is the instructors version
// -----------------------------------------------
void timerWaitUs_irq (uint32_t delayInMicroSeconds) {

  uint32_t       quotient, remainder;
  uint16_t       delta, new_cnt, count, underflow_amount;
  uint32_t       comp0, timerPeriodInMicroSeconds;
//  uint32_t       temp;

  // *******************************
  // range check the input and clamp
  // *******************************
  // check for 0/min...
  if (delayInMicroSeconds == 0) { return; } // just return if 0

  uint32_t frequency = CMU_ClockFreqGet (cmuClock_LETIMER0); // 1000 for EM3, 8192 for EM0 to 2
  uint16_t time_in_us_per_tick;

  // compute actual time/tick
  if (frequency == 1000)
    time_in_us_per_tick = 1000; // 1000us = 1ms
  else
    time_in_us_per_tick = 122;  // 122us = 1/8192

  // check for max, clamp to timer period if requested delay is greater than
  // the timer period
  comp0 = LETIMER_CompareGet (LETIMER0, 0);
  timerPeriodInMicroSeconds = comp0 * time_in_us_per_tick;

  if (delayInMicroSeconds > timerPeriodInMicroSeconds) {
    delayInMicroSeconds = timerPeriodInMicroSeconds;
    LOG_INFO ("Requested delay is > than LETIMER0 period, clqmped requested delay to %d\n", delayInMicroSeconds);
  }

  quotient  = delayInMicroSeconds / time_in_us_per_tick; // in number of ticks
  remainder = delayInMicroSeconds % time_in_us_per_tick; // in number of ticks

  delta = quotient + (remainder ? 1 : 0);

  count = (uint16_t) LETIMER_CounterGet (LETIMER0);

  if (count >= delta) {
    // non wrap-around case
    new_cnt = count - delta;
  } else {
      // wrap-around case
      //   numeric example:
      //     delayInMicroSeconds = 2,000,000 (or 2,000 ms)
      //     comp0               = 3,000 (for the ULFRCO)
      //     delta               = 2,000
      //     count               =   500 (LETIMER0 CNT)
      // so this subtraction will wrap around a 16-bit unsigned 2's comp value
      // (count - delta) = 500 - 2,000 = -1499 or 0xFA24
      // so the value 0xFFFF - (0xFA24 + 1) = 0x5DC or 1500, which is the
      // underflow amount, or how far we wrapped around.
      // So finally we have 3,000 - 1,500 = 1,500 which is the CNT value we
      // want to interrupt on to indicate that the delayInMicroSeconds has
      // expired/passed.
    underflow_amount = 0xFFFF - (count - delta + 1);
    new_cnt = comp0 - underflow_amount;
  }


  // set COMP1 to value for the computed delay amount
  LETIMER_CompareSet (LETIMER0, 1, new_cnt); // COMP1
//  temp = LETIMER_CompareGet (LETIMER0, 1);

  LETIMER_IntClear  (LETIMER0, LETIMER_IFC_COMP1);

  // enable COMP1 interrupts, NVIC IRQs for LETIMER0 are already enabled, see app.c
  LETIMER_IntEnable (LETIMER0, LETIMER_IEN_COMP1);
  LETIMER0->IEN = LETIMER0->IEN | LETIMER_IEN_COMP1; // DOS fix for compiler bug

  // ########################################################################
  /*
   * This code is here because of a weird compiler issue when using GCC 7.2.1
   * The call: LETIMER_IntEnable (LETIMER0, LETIMER_IEN_COMP1); did not set the
   * COMP1 IEN bit??? Now that we're on 10.2, testing showed that this wasn't required.
   */
  /*
  LETIMER_TypeDef    *letimer;
  uint32_t           ien;
  letimer = LETIMER0;
  ien = letimer->IEN; // fetch the IEN bits

  // ASSERT
  if (ien != 6) {
      LOG_ERROR("comp1=%d ien=%d", (int) temp, (int) ien);
      temp = LETIMER_CounterGet (LETIMER0);
      temp = LETIMER_CounterGet (LETIMER0);
      temp = LETIMER_CounterGet (LETIMER0);
      // force it
      letimer->IEN |= LETIMER_IEN_COMP1;
      ien = letimer->IEN; // fetch the IEN bits
      if (ien != 6) {
          LOG_ERROR("ASSERT: ien != 6");
          __BKPT(0);
      }
  }
  */
  // ########################################################################


  return;

} // timerWaitUs_irq()









void timerWaitUs_polled(uint32_t us_wait)
{
  uint32_t current_tick, delay_tick;
  uint32_t ticks_required = ((us_wait*ACTUAL_CLK_FREQ)/(1000*1000));          /* Number of ticks required*/

  if (ticks_required > (uint32_t)VALUE_TO_LOAD_COMP0)         /* Clamping the delay to LE timer period*/
    {
      ticks_required = (uint32_t)(VALUE_TO_LOAD_COMP0);
      LOG_ERROR("Delay requested is longer than time period; Clamping delay to LE timer time period \n\r");
    }
  else if (ticks_required < (uint32_t)(MIN_VALUE))            /* Clamping the delay to LE timer resolution*/
    {
      ticks_required = (uint32_t)(MIN_VALUE);
      LOG_ERROR("Delay requested is shorter than 1ms; Clamping delay to LE timer resolution \n\r");
    }


      current_tick = LETIMER_CounterGet(LETIMER0);
      delay_tick = current_tick - ticks_required;           /* LE timer is a countdown timer*/

  while((LETIMER_CounterGet(LETIMER0)) != (delay_tick));

}
