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

#define INCLUDE_LOG_DEBUG 0
#include "src/log.h"

#define ACTUAL_CLK_FREQ CMU_ClockFreqGet(cmuClock_LETIMER0)                    // frequency of the selected oscillator
//#define ACTUAL_CLK_FREQ (32768/4)
#define VALUE_TO_LOAD_COMP0 ((LETIMER_PERIOD_MS*ACTUAL_CLK_FREQ)/1000)        // 3000 ms
#define MIN_VALUE (1)


const LETIMER_Init_TypeDef LETIMER_INIT_VALUES =
  {
    false,              /* Do not Enable timer when initialization completes. */
    false,             /* Do stop counter during debug halt. */
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
  //uint32_t temp = LETIMER_IEN_COMP1 | LETIMER_IEN_UF;                  /* Attributions: Devang*/
  uint32_t temp = LETIMER_IEN_UF; // DOS Just UF

  // Enable LETIMER Interrupt
  LETIMER_IntEnable (LETIMER0, temp);


  LETIMER_Enable (LETIMER0, true);                  /* Start/Enable the timer */
}


void timerWaitUs_interrupt(uint32_t us_wait)
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

  //current=1000-4000
  //(0x5000)-(0x4000-0x1000)=0x2000.

      current_tick = LETIMER_CounterGet(LETIMER0);

      if (current_tick >= ticks_required){
      delay_tick = current_tick - ticks_required;           /* LE timer is a countdown timer*/
      }

      else{                    // overflow condition
        delay_tick = (VALUE_TO_LOAD_COMP0 - (ticks_required - current_tick));
      }

      LETIMER_CompareSet(LETIMER0, 1, delay_tick);          /* Loading the delay period in COMP1 register*/

      // DOS: Set COMP1 IEN bit, cleared in your LETIMER0_IRQHandler() if COMP1 IF bit is set.
      //      You have this commented out in irq.c: LETIMER_IntDisable(LETIMER0, LETIMER_IEN_COMP1);
      LETIMER_IntEnable (LETIMER0, LETIMER_IEN_COMP1);
      LETIMER0->IEN = LETIMER0->IEN | LETIMER_IEN_COMP1; // DOS fix for compiler bug

}

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
