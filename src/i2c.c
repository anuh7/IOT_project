/* @file      i2c.c
 * @version   1.0
 * @brief     I2C driver using I2CCSPM APIs and functions to measure temperature from on-board Si7021 device
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
 * @resources  https://community.silabs.com/s/article/how-to-perform-the-i2c-transfer-with-i2cspm-driver?language=en_US
 */

#include "em_i2c.h"
#include "em_gpio.h"
#include "sl_i2cspm.h"
#include "app.h"
#include "src/gpio.h"
#include "src/timers.h"

#define INCLUDE_LOG_DEBUG 0
#include "src/log.h"

#include "i2c.h"


#define SI7021_DEVICE_ADDR 0x40


/* Global variables */
I2C_TransferSeq_TypeDef transferSequence;
uint8_t cmd_data;
uint8_t read_data[2];
uint16_t temp_value;


void initI2C(void)
{

        I2CSPM_Init_TypeDef I2C_Config = {          /* Initialize the I2C hardware */
        .port = I2C0,
        .sclPort = gpioPortC,
        .sclPin = 10,
        .sdaPort = gpioPortC,
        .sdaPin = 11,
        .portLocationScl = 14,
        .portLocationSda = 16,
        .i2cRefFreq = 0,
        .i2cMaxFreq = I2C_FREQ_STANDARD_MAX,
        .i2cClhr = i2cClockHLRStandard
        };

         I2CSPM_Init(&I2C_Config);
}


void i2c_write()
{

  I2C_TransferReturn_TypeDef transferStatus;

  cmd_data = 0xF3;                  /* Measure Temperature, No Hold Master Mode */

  transferSequence.addr = SI7021_DEVICE_ADDR << 1;      // shift device address left
  transferSequence.flags = I2C_FLAG_WRITE;              // Indicate plain write sequence: S+ADDR(W)+DATA0+P.
  transferSequence.buf[0].data = &cmd_data;             // pointer to data
  transferSequence.buf[0].len = sizeof(cmd_data);

  transferStatus = I2C_TransferInit (I2C0, &transferSequence);    // Prepare and start an I2C transfer (single master mode only)

  if (transferStatus < 0)
      LOG_ERROR("I2C_TransferInit() Write error = %d", transferStatus);

  NVIC_EnableIRQ(I2C0_IRQn);              // Enabling the interrupt in NVIC
}

void i2c_read()
{
  I2C_TransferReturn_TypeDef transferStatus;

  transferSequence.addr = SI7021_DEVICE_ADDR << 1;          // shift device address left
  transferSequence.flags = I2C_FLAG_READ;                   // Indicate plain read sequence: S+ADDR(R)+DATA0+P.
  transferSequence.buf[0].data = &read_data[0];             // pointer to data to write into
  transferSequence.buf[0].len = sizeof(read_data);

  transferStatus = I2C_TransferInit (I2C0, &transferSequence);    // Prepare and start an I2C transfer (single master mode only).

  if (transferStatus < 0)
      LOG_ERROR("I2C_TransferInit() Read error = %d", transferStatus);

}


void read_temp_from_si7021()
{
    float temperature;
    temp_value = (read_data[0] << 8) + read_data[1];
    temperature = (((175.72*temp_value)/65536) - 46.85);

    LOG_INFO("Temperature = %f C\n\r", temperature);
}

