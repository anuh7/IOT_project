/* @file      timers.c
 * @version   1.0
 * @brief     Function to initialise the low energy timer
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

#include "em_letimer.h"
#include "app.h"
#include "em_cmu.h"

#include "src/oscillators.h"
#include "src/timers.h"

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#define ACTUAL_CLK_FREQ CMU_ClockFreqGet(cmuClock_LETIMER0)
#define VALUE_TO_LOAD_COMP0 ((LETIMER_PERIOD_MS*ACTUAL_CLK_FREQ)/1000)        //2250ms
#define VALUE_TO_LOAD_COMP1 ((LETIMER_OFF_TIME_MS*ACTUAL_CLK_FREQ)/1000)      //175ms

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
  LETIMER_Init(LETIMER0, &LETIMER_INIT_VALUES);

  LETIMER_CompareSet(LETIMER0, 0, VALUE_TO_LOAD_COMP0);
  LETIMER_CompareSet(LETIMER0, 1, VALUE_TO_LOAD_COMP1);

  LETIMER_IntClear (LETIMER0, 0xFFFFFFFF);

  LETIMER_IntEnable(LETIMER0, LETIMER_IEN_UF | LETIMER_IEN_COMP1);
  LETIMER_Enable (LETIMER0, true);
}
