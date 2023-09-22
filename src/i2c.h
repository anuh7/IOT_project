/* @file      i2c.h
 * @version   1.0
 * @brief     Application Interface for I2C driver
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

#ifndef SRC_I2C_H_
#define SRC_I2C_H_

/**
* @brief Function to initiate i2c write sequence to device
*
* I2C transfer sequence passes the transfer message structure which holds the device address,
* buffer to hold data to send from master, and sequence type
*
* @param void
* @return void
*/
void i2c_write();

/**
* @brief Function to initiate i2c read sequence to device
*
* I2C transfer sequence passes the transfer message structure which holds the device address,
* buffer to hold data to receive into from device, and sequence type
*
* @param void
* @return void
*/
void i2c_read();

/**
* @brief Function to calculate temperature from the 16-bit word returned by Si7021
*
*
* @param void
* @return float   Returns the temperature value in Celsius
*/
float measureTemp();

/**
* @brief Function initialises I2C peripheral, calls i2c_write and i2c_read functions
*
* The device is ON by making the device SENSOR_ENABLE pin HIGH, and I2C is initialised. i2c_write
* and i2c_read are called with sufficient delay and the temperature value is logged on
* the serial terminal
*
* @param void
* @return void
*/
void read_temp_from_si7021();

#endif /* SRC_I2C_H_ */
