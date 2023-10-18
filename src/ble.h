/* @file      ble.h
 * @version   1.0
 * @brief     Application interface provided for ble.c
 *
 * @author    Anuhya Kuraparthy, anuhya.kuraparthy@colorado.edu
 * @date      Oct 13, 2023
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823: IoT Embedded Firmware
 * @instructor  David Sluiter
 *
 * @assignment Assignment 6 - LCD Integration and Client Command Table for A7
 * @due        Oct 13
 *
 * @resources  -
 */

#ifndef SRC_BLE_H_
#define SRC_BLE_H_

#include <stdbool.h>
#include <stdint.h>
#include "src/ble_device_type.h"
#include "sl_bluetooth.h"
#include "sl_bt_api.h"


// Helper Macros
#define UINT8_TO_BITSTREAM(p, n) { *(p)++ = (uint8_t)(n); } // use this for the flags byte, which you set = 0
#define UINT32_TO_BITSTREAM(p, n) { *(p)++ = (uint8_t)(n); *(p)++ = (uint8_t)((n) >> 8); \
    *(p)++ = (uint8_t)((n) >> 16); *(p)++ = (uint8_t)((n) >> 24); }
#define UINT32_TO_FLOAT(m, e) (((uint32_t)(m) & 0x00FFFFFFU) | (uint32_t)((int32_t)(e) << 24))

typedef struct {
      // values that are common to servers and clients
      bd_addr myAddress;
      uint8_t myAddressType;

      // values unique for server
      uint8_t advertisingSetHandle;
      uint8_t connection_handle;
      bool connection_open;             // true when in an open connection
      bool ok_to_send_htm_indications;  // true when client enabled indications
      bool indication_in_flight;        // true when an indication is in-flight

      //client
      uint32_t service_handle;
      //characteristic handle ie. address
      uint16_t characteristic;
      uint8_t * characteristic_value;

} ble_data_struct_t;


/**
* @brief Function to pass the temperature read from Si7021 as a GATT attribute
*         to the client.
*
* @param void
* @return void
*/
void send_temperature();

/**
* @brief Function that returns a pointer to the BLE private data so it can be
*         used in other .c files as well.
*
* @param void
* @return sl_bt_msg_t BLE event
*/
ble_data_struct_t* getBleDataPtr(void);

/**
* @brief Function to handle BLE events.
*
* @param sl_bt_msg_t BLE event
* @return void
*/
void handle_ble_event(sl_bt_msg_t *evt);


#endif /* SRC_BLE_H_ */
