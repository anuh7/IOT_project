/*
 * ble.c
 *
 *  Created on: 03-Oct-2023
 *      Author: ROTODYNE
 */


#include "src/ble.h"
#include "sl_bluetooth.h"
#include "sl_bt_api.h"
#include "src/ble_device_type.h"

#include "gatt_db.h"
#include "sl_status.h"

#include "src/i2c.h"

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

// BLE private data
ble_data_struct_t ble_data; // this is the declaration
sl_status_t sc = 0;


// function that returns a pointer to the BLE private data
ble_data_struct_t* getBleDataPtr() {
    return (&ble_data);
} // getBleDataPtr()

void send_temperature()
{
  uint8_t htm_temperature_buffer[5];
  uint8_t *p = htm_temperature_buffer;
  uint32_t htm_temperature_flt;
  int32_t temperature_in_c;
  uint8_t flags = 0x00;

  ble_data_struct_t *bleDataPtr = getBleDataPtr();

  if (bleDataPtr->connection_open == true){

      temperature_in_c = (int32_t)read_temp_from_si7021();

      UINT8_TO_BITSTREAM(p, flags);

      htm_temperature_flt = UINT32_TO_FLOAT(temperature_in_c*1000, -3);
      // Convert temperature to bitstream and place it in the htm_temperature_buffer
      UINT32_TO_BITSTREAM(p, htm_temperature_flt);

      sc = sl_bt_gatt_server_write_attribute_value(
          gattdb_temperature_measurement,
          0,
          4,
          (const uint8_t *)&temperature_in_c);

      if (sc != SL_STATUS_OK) {
                   LOG_ERROR("sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x", (unsigned int) sc);
               }

      if(bleDataPtr->ok_to_send_htm_indications == true && bleDataPtr->indication_in_flight == false){

          sc = sl_bt_gatt_server_send_indication(bleDataPtr->connection_handle,
                                                 gattdb_temperature_measurement,
                                                 5,
                                                 &htm_temperature_buffer[0]);

          if (sc != SL_STATUS_OK) {
                       LOG_ERROR("sl_bt_gatt_server_send_indication() returned != 0 status=0x%04x", (unsigned int) sc);
                   }
          else{
              bleDataPtr->indication_in_flight = true;
          }

        }
  }
}

void handle_ble_event(sl_bt_msg_t *evt) {

  ble_data_struct_t *bleDataPtr = getBleDataPtr();

  switch (SL_BT_MSG_ID(evt->header)) {
// ******************************************************
// Events common to both Servers and Clients
// ******************************************************
// --------------------------------------------------------
// This event indicates the device has started and the radio is ready.
// Do not call any stack API commands before receiving this boot event!
// Including starting BT stack soft timers!
// --------------------------------------------------------
    case sl_bt_evt_system_boot_id:
          LOG_INFO("Boot Evt\n"); // DOS

          sc = sl_bt_system_get_identity_address(&(bleDataPtr->myAddress), &(bleDataPtr->myAddressType));

          if (sc != SL_STATUS_OK) {
              LOG_ERROR("sl_bt_system_get_identity_address() returned != 0 status=0x%04x", (unsigned int) sc);
          }

          sc = sl_bt_advertiser_create_set(&(bleDataPtr->advertisingSetHandle));

          if (sc != SL_STATUS_OK) {
              LOG_ERROR("sl_status_t sl_bt_advertiser_create_set() returned != 0 status=0x%04x", (unsigned int) sc);
          }

          sc = sl_bt_advertiser_set_timing(
              (bleDataPtr->advertisingSetHandle),
              400,                  // Min. advertising interval (250 ms *(1/0.625))
              400,                  // Max. advertising interval (250 ms *(1/0.625))
              0,
              0);

          if (sc != SL_STATUS_OK) {
              LOG_ERROR("sl_bt_advertiser_set_timing() returned != 0 status=0x%04x", (unsigned int) sc);
          }

          sc = sl_bt_advertiser_start((bleDataPtr->advertisingSetHandle),
                                      sl_bt_advertiser_general_discoverable,
                                      sl_bt_advertiser_connectable_scannable);

          if (sc != SL_STATUS_OK) {
              LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x", (unsigned int) sc);
          }

          bleDataPtr->connection_open = false;
          bleDataPtr->indication_in_flight = false;
          bleDataPtr->ok_to_send_htm_indications = false;

      break;

    case sl_bt_evt_connection_opened_id:
      LOG_INFO("Open Evt\n"); // DOS

      // handle open event
      bleDataPtr->connection_open = true;
      bleDataPtr->connection_handle = evt->data.evt_connection_opened.connection;

      sc = sl_bt_advertiser_stop((bleDataPtr->advertisingSetHandle));

      if (sc != SL_STATUS_OK) {
                    LOG_ERROR("sl_bt_advertiser_stop() returned != 0 status=0x%04x", (unsigned int) sc);
      }

//      ******************************************************************************/
//     sl_status_t sl_bt_connection_set_parameters(uint8_t connection,
//                                                 uint16_t min_interval,
//                                                 uint16_t max_interval,
//                                                 uint16_t latency,
//                                                 uint16_t timeout,
//                                                 uint16_t min_ce_length,
//                                                 uint16_t max_ce_length);
//
//     /***************************************************************************//**
      sc = sl_bt_connection_set_parameters(bleDataPtr->connection_handle,
                                           0x3c,          //75/1.25
                                           0x3c,
                                           0x04,          //300ms
                                           0x50,          //800/10
                                      0,
                                      0);
      if (sc != SL_STATUS_OK) {
                    LOG_ERROR("sl_bt_connection_set_parameters() returned != 0 status=0x%04x", (unsigned int) sc);
      }

      break;

     case sl_bt_evt_connection_closed_id:
       LOG_INFO("Close Evt\n"); // DOS

       // handle close event
       bleDataPtr->connection_open = false;

       sc = sl_bt_advertiser_start((bleDataPtr->advertisingSetHandle),
                                   sl_bt_advertiser_general_discoverable,
                                   sl_bt_advertiser_connectable_scannable);

       if (sc != SL_STATUS_OK) {
           LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x", (unsigned int) sc);
       }

       break;

     case sl_bt_evt_connection_parameters_id:

       LOG_INFO("Connection parameters: Connection=%d \n\r", (int)(evt->data.evt_connection_parameters.connection));
       LOG_INFO("Interval=%d \n\r", (int)(evt->data.evt_connection_parameters.interval*1.25));
       LOG_INFO("Latency=%d \n\r", (int)(evt->data.evt_connection_parameters.latency));
       LOG_INFO("Timeout=%d \n\r", (int)(evt->data.evt_connection_parameters.timeout*10));

       break;

     case sl_bt_evt_system_external_signal_id:

       break;

     case sl_bt_evt_gatt_server_characteristic_status_id:

       if (evt->data.evt_gatt_server_characteristic_status.connection == gattdb_temperature_measurement)
         {
           // Characteristic client configuration has been changed.

           if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config)
             {
                 if (evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_server_disable)
                   bleDataPtr->ok_to_send_htm_indications = false;

                 if (evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_server_indication)
                   bleDataPtr->ok_to_send_htm_indications = true;
             }

           if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_confirmation)
             {
               bleDataPtr->indication_in_flight = false;
             }

         }

       break;

     case sl_bt_evt_gatt_server_indication_timeout_id:

       bleDataPtr->indication_in_flight = false;
       break;

  } // end - switch
} // handle_ble_event()
