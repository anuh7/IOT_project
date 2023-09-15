/* @file      oscillators.c
 * @version   1.0
 * @brief     Function to initialize the oscillator
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
#include "app.h"
#include "em_cmu.h"

#include "src/oscillators.h"


void oscillator_init()
{
  if (LOWEST_ENERGY_MODE == 3)
    {
      CMU_OscillatorEnable (cmuOsc_ULFRCO,true, true);
      CMU_ClockSelectSet(cmuClock_LFA,cmuSelect_ULFRCO);
      CMU_ClockDivSet(cmuClock_LETIMER0, cmuClkDiv_1);
      CMU_ClockEnable(cmuClock_LETIMER0, true);
    }
  else
    {
      CMU_OscillatorEnable (cmuOsc_LFXO,true, true);
      CMU_ClockSelectSet(cmuClock_LFA,cmuSelect_LFXO);
      CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
      CMU_ClockDivSet(cmuClock_LETIMER0, cmuClkDiv_4);      //selecting prescalar of 4
      CMU_ClockEnable(cmuClock_LETIMER0, true);
    }
}

