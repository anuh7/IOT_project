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

  // Set UF in LETIMER0_IEN, so that the timer will generate IRQs to the NVIC.
  uint32_t temp = LETIMER_IEN_UF;                     /* Attribution: Instructor Dave Sluiter*/

  // Enable LETIMER Interrupt
  LETIMER_IntEnable (LETIMER0, temp);

  LETIMER_Enable (LETIMER0, true);                  /* Start/Enable the timer */

}


void timerWaitUs_interrupt(uint32_t us_wait)
{
  uint16_t current_tick, underflow_amount;
  uint16_t time_in_us_per_tick, ticks_required;
  uint32_t comp0_value, timerPeriodInMicroSeconds;
  uint32_t quotient;

  uint32_t frequency = CMU_ClockFreqGet (cmuClock_LETIMER0); // 1000 for EM3, 8192 for EM0 to 2

  // compute actual time/tick
  if (frequency == 1000)
    time_in_us_per_tick = 1000; // 1000us = 1ms
  else
    time_in_us_per_tick = 122;  // 122us = 1/8192         /* Attribution: Instructor Dave Sluiter*/

  comp0_value = LETIMER_CompareGet (LETIMER0, 0);
  timerPeriodInMicroSeconds = comp0_value * time_in_us_per_tick;

  // range check
  if (us_wait > timerPeriodInMicroSeconds)         /* Clamping the delay to LE timer period */
    {
      us_wait = (timerPeriodInMicroSeconds);
      LOG_ERROR("Delay requested is longer than time period; Clamping delay to LE timer time period \n\r");
    }

  quotient  = us_wait / time_in_us_per_tick; // in number of ticks
  current_tick = (uint16_t) LETIMER_CounterGet(LETIMER0);

      // non-wrap around case
      if (current_tick > quotient){
          ticks_required = current_tick - quotient;
      }
      else{   // wrap-around case
          underflow_amount = 0xFFFF - (current_tick - quotient + 1);    /* Attribution: Instructor Dave Sluiter*/
          ticks_required = comp0_value - underflow_amount;
      }

      LETIMER_CompareSet(LETIMER0, 1, ticks_required);     /* Loading the delay period in COMP1 register*/
      LETIMER_IntClear  (LETIMER0, LETIMER_IFC_COMP1);

      // DOS: Set COMP1 IEN bit, cleared in your LETIMER0_IRQHandler() if COMP1 IF bit is set.
      LETIMER_IntEnable (LETIMER0, LETIMER_IEN_COMP1);
      LETIMER0->IEN = LETIMER0->IEN | LETIMER_IEN_COMP1;  /* Attribution: Instructor Dave Sluiter*/

}




/* Attribution: Instructor Dave Sluiter*/
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
    time_in_us_per_tick = 1000; // 1000us = 1ms = 1/1000
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
