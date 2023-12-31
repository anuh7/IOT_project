/* @file      ble.h
 * @version   1.0
 * @brief     Application interface provided for ble.c
 *
 * @author    Anuhya Kuraparthy, anuhya.kuraparthy@colorado.edu
 * @date      Oct 20, 2023
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823: IoT Embedded Firmware
 * @instructor  David Sluiter
 *
 * @assignment Assignment 7 - Bluetooth BLE Client
 * @due        Oct 20
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
  uint8_t        buffer[5];     // The actual data buffer for the indication.
  uint16_t       charHandle;    // Char handle from gatt_db.h
  size_t        bufferLength;    // Length of buffer in bytes to send
} queue_struct_t;

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
      uint32_t passkey;


      //A8
      bool button_indication;
      bool htm_indication;
      bool button_status;
      bool pb1_button_status;
      bool bonded;
      uint8_t queued_indications;
      bool button_indication_client;

      //client
      uint32_t service_handle;
      uint16_t characteristic;          //characteristic handle ie. address
      uint8_t * characteristic_value;

      uint32_t button_service_handle;
      uint16_t button_characteristic;          //characteristic handle ie. address
      uint8_t * button_characteristic_value;

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
