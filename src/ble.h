/*
 * ble.h
 *
 *  Created on: 03-Oct-2023
 *      Author: ROTODYNE
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

      bool connection_open; // true when in an open connection
      bool ok_to_send_htm_indications; // true when client enabled indications
      bool indication_in_flight; // true when an indication is in-flight
      // values unique for client
} ble_data_struct_t;


// function prototypes
void send_temperature();
ble_data_struct_t* getBleDataPtr(void);
void handle_ble_event(sl_bt_msg_t *evt);


#endif /* SRC_BLE_H_ */
