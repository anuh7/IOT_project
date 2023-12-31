/* @file      oscillators.c
 * @version   2.0
 * @brief     Function to initialize the oscillator
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

#define INCLUDE_LOG_DEBUG 0
#include "src/log.h"


#include <stdint.h>
#include "app.h"
#include "em_cmu.h"

#include "src/oscillators.h"


void oscillator_init()
{
  if (LOWEST_ENERGY_MODE == 3)
    {
      CMU_OscillatorEnable (cmuOsc_ULFRCO,true, true);        /* ULFRCO oscillator is used for EM3 energy mode*/
      CMU_ClockSelectSet(cmuClock_LFA,cmuSelect_ULFRCO);      /* Selecting the clock for the low frequency peripherals*/
      CMU_ClockDivSet(cmuClock_LETIMER0, cmuClkDiv_1);        /* Setting the prescalar */
      CMU_ClockEnable(cmuClock_LETIMER0, true);
    }
  else
    {
      CMU_OscillatorEnable (cmuOsc_LFXO,true, true);        /* LFXO oscillator is used for rest of energy mode*/
      CMU_ClockSelectSet(cmuClock_LFA,cmuSelect_LFXO);
      CMU_ClockDivSet(cmuClock_LETIMER0, cmuClkDiv_4);      /* Selecting prescalar of 4 */
      CMU_ClockEnable(cmuClock_LETIMER0, true);
    }

  // DOS: See if we have the right frequency
  uint32_t frequency = CMU_ClockFreqGet (cmuClock_LFA); // got 32768    /* Attribution: Instructor Dave Sluiter*/
  LOG_INFO("LFA clock freq = %d", (int) frequency);
  frequency = CMU_ClockFreqGet (cmuClock_LETIMER0); // got 8129
  LOG_INFO("LETIMER0 clock freq = %d", (int) frequency);

}

