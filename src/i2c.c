/* @file      i2c.c
 * @version   1.0
 * @brief     I2C driver using I2CCSPM APIs and functions to measure temperature from on-board Si7021 device
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
 * @resources  https://community.silabs.com/s/article/how-to-perform-the-i2c-transfer-with-i2cspm-driver?language=en_US
 */

#include "em_i2c.h"
#include "em_gpio.h"
#include "sl_i2cspm.h"
#include "app.h"
#include "src/gpio.h"
#include "src/timers.h"

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#include "i2c.h"


#define SI7021_DEVICE_ADDR 0x40


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

/* Global variables */

I2C_TransferReturn_TypeDef transferStatus;

uint8_t cmd_data;
uint8_t read_data[2];
uint16_t temp_value;


typedef struct{
    float last_temp_measurement;              /* Structure to hold temperature measurement*/
}si7021_data;

si7021_data measurements;

void i2c_write()
{
  I2C_TransferSeq_TypeDef transferSequence;

  cmd_data = 0xF3;                  /* Measure Temperature, No Hold Master Mode */

  transferSequence.addr = SI7021_DEVICE_ADDR << 1;      // shift device address left
  transferSequence.flags = I2C_FLAG_WRITE;              // Indicate plain write sequence: S+ADDR(W)+DATA0+P.
  transferSequence.buf[0].data = &cmd_data;             // pointer to data
  transferSequence.buf[0].len = sizeof(cmd_data);

  transferStatus = I2CSPM_Transfer(I2C0, &transferSequence);
  if (transferStatus != i2cTransferDone){                 // error check
      LOG_ERROR("I2CSPM_Transfer: I2C bus write of cmd=0xF3 failed, return value=%d \n\r", (uint32_t)transferStatus);
  }
}

void i2c_read()
{
  I2C_TransferSeq_TypeDef transferSequence;


  transferSequence.addr = SI7021_DEVICE_ADDR << 1;          // shift device address left
  transferSequence.flags = I2C_FLAG_READ;                   // Indicate plain read sequence: S+ADDR(R)+DATA0+P.
  transferSequence.buf[0].data = &read_data[0];             // pointer to data to write into
  transferSequence.buf[0].len = sizeof(read_data);

  transferStatus = I2CSPM_Transfer (I2C0, &transferSequence);
  if (transferStatus != i2cTransferDone){
      LOG_ERROR("I2CSPM_Transfer: I2C bus read failed, return value=%d \n\r", (uint32_t)transferStatus);

  }
}

float measureTemp()
{
  float temperature;

  temp_value = (read_data[0] << 8) + read_data[1];
  temperature = (((175.72*temp_value)/65536) - 46.85);

  return temperature;
}


void read_temp_from_si7021()
{
   sensor_enable();                       // turn on power to Si7021 device
   I2CSPM_Init(&I2C_Config);              // Initialise I2C peripheral
   timerWaitUs(80000);                    // wait for 80ms for POR sequence

   i2c_write();                           // I2C write command
   timerWaitUs(12000);                    // Conversion time for 14-bit temperature value

   i2c_read();                            // I2C Read command
   sensor_disable();                      // Turn off device

   measurements.last_temp_measurement = measureTemp();
   LOG_INFO("Temperature = %f C\n\r", measurements.last_temp_measurement);
}

