/* @file      i2c.h
 * @version   1.0
 * @brief     Application Interface for I2C driver
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

#ifndef SRC_I2C_H_
#define SRC_I2C_H_

void initI2C(void);

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
* @brief Function to calculate temperature from the 16-bit word returned by Si7021 the temperature value is logged on
* the serial terminal
*
*
* @param void
* @return void
*/
void read_temp_from_si7021();

#endif /* SRC_I2C_H_ */
