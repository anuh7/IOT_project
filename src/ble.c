/* @file      ble.c
 * @version   1.0
 * @brief     Functions for BLE events
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

#include "src/ble.h"
#include "sl_bluetooth.h"
#include "sl_bt_api.h"
#include "src/ble_device_type.h"

#include "gatt_db.h"
#include "sl_status.h"
#include "math.h" // need function prototype for pow()

#include "src/i2c.h"
#include "src/lcd.h"

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

// BLE private data
ble_data_struct_t ble_data;
sl_status_t sc = 0;
bd_addr server_addr = SERVER_BT_ADDRESS;
int32_t temperature_from_server = 0;

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

      temperature_in_c = (uint32_t)read_temp_from_si7021();

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

      // Pass the GATT attribute only if the indications are set
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
              displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp=%d", temperature_in_c);
          }

        }
  }
}


static int32_t FLOAT_TO_INT32(const uint8_t *value_start_little_endian)
{
    uint8_t mantissaSignByte = 0; // these bits will become the upper 8-bits of the mantissa
    int32_t mantissa; // this holds the 24-bit mantissa value with the upper 8-bits as sign bits

      // input data format is:
      // [0] = flags byte
      // [3][2][1] = mantissa (2's complement)
      // [4] = exponent (2's complement)
      // BT value_start_little_endian[0] has the flags byte

    int8_t exponent = (int8_t)value_start_little_endian[4]; // the exponent is a signed 2’s comp value
    // sign extend the mantissa value if the mantissa is negative

    if (value_start_little_endian[3] & 0x80) { // msb of [3] is the sign of the mantissa
    mantissaSignByte = 0xFF;
    }
    // assemble the mantissa

    mantissa = (int32_t) (value_start_little_endian[1] << 0) |
    (value_start_little_endian[2] << 8) |
    (value_start_little_endian[3] << 16) |
    (mantissaSignByte << 24) ;

    // value = 10^exponent * mantissa; pow() returns a double type
    return (int32_t) (pow( (double) 10, (double) exponent) * (double) mantissa);
} // FLOAT_TO_INT32



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

      displayInit();

      // BOTH SERVER AND CLIENT ADDRESS
          sc = sl_bt_system_get_identity_address(&(bleDataPtr->myAddress), &(bleDataPtr->myAddressType));

          if (sc != SL_STATUS_OK) {
              LOG_ERROR("sl_bt_system_get_identity_address() returned != 0 status=0x%04x", (unsigned int) sc);
          }

#if DEVICE_IS_BLE_SERVER

          //SERVER

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

          displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");
          displayPrintf(DISPLAY_ROW_NAME, "Server");


#else
          //CLIENT

          // passive scanning and 1M PHY
          sc = sl_bt_scanner_set_mode(1, 0);
          if (sc != SL_STATUS_OK) {
              LOG_ERROR("sl_bt_scanner_set_mode() returned != 0 status=0x%04x", (unsigned int) sc);
          }

          sc = sl_bt_scanner_set_timing(1, 80, 40);
          if (sc != SL_STATUS_OK) {
              LOG_ERROR("sl_bt_scanner_set_timing() returned != 0 status=0x%04x", (unsigned int) sc);
          }

          sc = sl_bt_connection_set_default_parameters(60, 60, 4, 82, 0, 6.4); //max ce value = 4/0.625 =6.4???
          if (sc != SL_STATUS_OK) {
              LOG_ERROR("sl_bt_scanner_set_timing() returned != 0 status=0x%04x", (unsigned int) sc);
          }

          sc = sl_bt_scanner_start(1, sl_bt_scanner_discover_generic);
          if (sc != SL_STATUS_OK) {
              LOG_ERROR("sl_bt_scanner_set_timing() returned != 0 status=0x%04x", (unsigned int) sc);
          }

          displayPrintf(DISPLAY_ROW_NAME, "Client");
          displayPrintf(DISPLAY_ROW_CONNECTION, "Scanning");

#endif

          displayPrintf(DISPLAY_ROW_BTADDR, "%02X:%02X:%02X:%02X:%02X:%02X",
                        bleDataPtr->myAddress.addr[0],
                        bleDataPtr->myAddress.addr[1],
                        bleDataPtr->myAddress.addr[2],
                        bleDataPtr->myAddress.addr[3],
                        bleDataPtr->myAddress.addr[4],
                        bleDataPtr->myAddress.addr[5]);
          displayPrintf(DISPLAY_ROW_ASSIGNMENT, "A7");



          // Initialising the flags
          bleDataPtr->connection_open = false;
          bleDataPtr->indication_in_flight = false;
          bleDataPtr->ok_to_send_htm_indications = false;

      break;

    case sl_bt_evt_connection_opened_id:

      // handle open event
      bleDataPtr->connection_open = true;
      bleDataPtr->connection_handle = evt->data.evt_connection_opened.connection;

      //sc = sl_bt_gatt_discover_primary_services_by_uuid(evt->data.evt_connection_opened.connection,


#if DEVICE_IS_BLE_SERVER

          //SERVER

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


#else
      displayPrintf(DISPLAY_ROW_BTADDR2,  "%02X:%02X:%02X:%02X:%02X:%02X",
                    server_addr.addr[0],
                    server_addr.addr[1],
                    server_addr.addr[2],
                    server_addr.addr[3],
                    server_addr.addr[4],
                    server_addr.addr[5]);

#endif

      displayPrintf(DISPLAY_ROW_CONNECTION, "Connected");

      break;

     case sl_bt_evt_connection_closed_id:

       // handle close event
       bleDataPtr->connection_open = false;
//       bleDataPtr->indication_in_flight = false;
//       bleDataPtr->ok_to_send_htm_indications = false;

#if DEVICE_IS_BLE_SERVER
       sc = sl_bt_advertiser_start((bleDataPtr->advertisingSetHandle),
                                   sl_bt_advertiser_general_discoverable,       // Discoverable using general discovery procedure
                                   sl_bt_advertiser_connectable_scannable);     // Undirected connectable scannable

       if (sc != SL_STATUS_OK) {
           LOG_ERROR("sl_bt_advertiser_start() returned != 0 status=0x%04x", (unsigned int) sc);
       }

       displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");
#else

       sc = sl_bt_scanner_start(1, sl_bt_scanner_discover_generic);
       if (sc != SL_STATUS_OK) {
           LOG_ERROR("sl_bt_scanner_set_timing() returned != 0 status=0x%04x", (unsigned int) sc);
       }
       displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");

#endif

       displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
       displayPrintf(DISPLAY_ROW_BTADDR2, "");

       break;

     case sl_bt_evt_connection_parameters_id:

//       LOG_INFO("Connection parameters: Connection=%d \n\r", (int)(evt->data.evt_connection_parameters.connection));
//       LOG_INFO("Interval=%d \n\r", (int)(evt->data.evt_connection_parameters.interval*1.25));
//       LOG_INFO("Latency=%d \n\r", (int)(evt->data.evt_connection_parameters.latency));
//       LOG_INFO("Timeout=%d \n\r", (int)(evt->data.evt_connection_parameters.timeout*10));

       break;

     case sl_bt_evt_system_external_signal_id:

       break;

     case sl_bt_evt_system_soft_timer_id:
       displayUpdate();
       break;


#if DEVICE_IS_BLE_SERVER

     case sl_bt_evt_gatt_server_characteristic_status_id:


       // Checking for the correct characteristic
       if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_temperature_measurement)
         {
           if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config)
             {
                 if (evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_server_disable){
                   bleDataPtr->ok_to_send_htm_indications = false;            // Notifications are disabled
                   displayPrintf(DISPLAY_ROW_TEMPVALUE, " ");
                 }

                 if (evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_server_indication){
                   bleDataPtr->ok_to_send_htm_indications = true;             // Client has enabled notifications
                 }
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

#else

     case sl_bt_evt_scanner_scan_report_id:

//bd_addr    address;           /**< Bluetooth address of the remote device */
//uint8_t    address_type;      /**< Advertiser address type. Values:

       if ( (evt->data.evt_scanner_scan_report.packet_type == 0) &&
           (evt->data.evt_scanner_scan_report.address_type == 0) &&
           (evt->data.evt_scanner_scan_report.address.addr[0] == server_addr.addr[0]) &&
           (evt->data.evt_scanner_scan_report.address.addr[1] == server_addr.addr[1]) &&
           (evt->data.evt_scanner_scan_report.address.addr[2] == server_addr.addr[2]) &&
           (evt->data.evt_scanner_scan_report.address.addr[3] == server_addr.addr[3]) &&
           (evt->data.evt_scanner_scan_report.address.addr[4] == server_addr.addr[4]) &&
           (evt->data.evt_scanner_scan_report.address.addr[5] == server_addr.addr[5]) )
         {
           sc = sl_bt_scanner_stop();
           if (sc != SL_STATUS_OK) {
               LOG_ERROR("sl_bt_scanner_stop() returned != 0 status=0x%04x", (unsigned int) sc);
           }

           sc = sl_bt_connection_open(evt->data.evt_scanner_scan_report.address,
                                      sl_bt_gap_public_address,
                                      sl_bt_gap_phy_1m,
                                      NULL);
           if (sc != SL_STATUS_OK) {
               LOG_ERROR("sl_bt_connection_open() returned != 0 status=0x%04x", (unsigned int) sc);
           }

         }

       break;

     case sl_bt_evt_gatt_procedure_completed_id:

       break;

     case sl_bt_evt_gatt_service_id:

       bleDataPtr->service_handle = evt->data.evt_gatt_service.service;
       break;

     case sl_bt_evt_gatt_characteristic_id:

       bleDataPtr->characteristic = evt->data.evt_gatt_characteristic.characteristic;
       break;

     case sl_bt_evt_gatt_characteristic_value_id:

//       PACKSTRUCT( struct sl_bt_evt_gatt_characteristic_value_s
//       {
//         uint8_t    connection;     /**< Connection handle */
//         uint16_t   characteristic; /**< GATT characteristic handle. This value is
//                                         normally received from the gatt_characteristic
//                                         event. */
//         uint8_t    att_opcode;     /**< Enum @ref sl_bt_gatt_att_opcode_t. Attribute
//                                         opcode, which indicates the GATT transaction
//                                         used. */
//         uint16_t   offset;         /**< Value offset */
//         uint8array value;          /**< Characteristic value */
//       });

       if (evt->data.evt_gatt_characteristic_value.characteristic == bleDataPtr->characteristic)
         {
           sc = sl_bt_gatt_send_characteristic_confirmation(bleDataPtr->connection_handle);
           if (sc != SL_STATUS_OK) {
               LOG_ERROR("sl_bt_gatt_send_characteristic_confirmation() returned != 0 status=0x%04x", (unsigned int) sc);
         }

           //Receiving the temperature indications from the Server and displaying it on the Client’s LCD.
        bleDataPtr->characteristic_value = &(evt->data.evt_gatt_characteristic_value.value.data[0]);
        temperature_from_server = FLOAT_TO_INT32(bleDataPtr->characteristic_value);
        displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp = %d",temperature_from_server);
         }


       break;

#endif

  } // end - switch
} // handle_ble_event()
