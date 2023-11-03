/* @file      ble.c
 * @version   1.0
 * @brief     Functions for BLE events
 *
 * @author    Anuhya Kuraparthy, anuhya.kuraparthy@colorado.edu
 * @date      Oct 28, 2023
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823: IoT Embedded Firmware
 * @instructor  David Sluiter
 *
 * @assignment Assignment 8: BLE Server with Security
 * @due        Oct 28
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
#include "src/gpio.h"
#include "src/scheduler.h"

#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

#define QUEUE_DEPTH      (16)

// BLE private data
ble_data_struct_t ble_data;
sl_status_t sc = 0;
bd_addr server_addr = SERVER_BT_ADDRESS;
int32_t temperature_from_server = 0;

queue_struct_t   my_queue[QUEUE_DEPTH]; // This is the storage for your queue

//circular buffer metadata
uint32_t         wptr = 0;              // write pointer
uint32_t         rptr = 0;              // read pointer
bool isFullflag = false;                // Flag to handle the full condition

queue_struct_t dequeued_element;

//typedef enum uint32_t {
//  PB0_pressed,
//  PB1_pressed,
//  PB1_released,
//  PB0_released
//}toggle_states;

#if !DEVICE_IS_BLE_SERVER

static const uint8_t characteristic_uuid[] =  { 0x1c, 0x2a }; // Reverse the byte order for little-endian format
static const uint8_t service_uuid[2] = { 0x09, 0x18 };

static const uint8_t button_characteristic_uuid[] =  { 0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x02, 0x00, 0x00, 0x00 }; // Reverse the byte order for little-endian format
static const uint8_t button_service_uuid[] =  { 0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x01, 0x00, 0x00, 0x00 };

#endif


// function that returns a pointer to the BLE private data
ble_data_struct_t* getBleDataPtr() {
    return (&ble_data);
} // getBleDataPtr()


static uint32_t nextPtr(uint32_t ptr)
{

  if(ptr+1 == QUEUE_DEPTH)
    return 0;
  else
    return ptr + 1;

} // nextPtr()


//typedef struct {
//  uint8_t        buffer[5];     // The actual data buffer for the indication.
//  uint16_t       charHandle;    // Char handle from gatt_db.h
//  size_t        bufferLength;    // Length of buffer in bytes to send
//} queue_struct_t;

bool write_queue (uint8_t a[], uint16_t b, size_t c)
{
    if(isFullflag)
    {
      LOG_INFO("\n Buffer is Full \n");
      return true;
    }
    else
    {
      my_queue[wptr].bufferLength = c;
      my_queue[wptr].charHandle = b;

      for (unsigned int i=0; i < c; i++){
        my_queue[wptr].buffer[i] = a[i];
      }

      if (nextPtr(wptr) == rptr)
      {
        isFullflag = true;      //write pointer suppression
      }
      else
      {
        wptr = nextPtr(wptr);
      }
    }

    return false;

} // write_queue()



// read_queue(
bool read_queue (uint8_t *a, uint16_t *b, size_t *c)
{

  if(wptr == rptr && isFullflag != 1)
  {
      LOG_INFO("\n Buffer is Empty \n");
      return true;
  }
  else
  {
      for (unsigned int i = 0; i < *c; i++) {
          a[i] = my_queue[rptr].buffer[i];
      }
    *b = my_queue[rptr].charHandle;
    *c = my_queue[rptr].bufferLength;

    rptr = nextPtr(rptr);

    if (isFullflag)                   //dequeue an element
    {
        isFullflag = false;           //clearing the flag
        wptr = nextPtr(wptr);         //incremeting the wptr
    }
  }

  return false;
} // read_queue()


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
      if(bleDataPtr->ok_to_send_htm_indications){

        if(bleDataPtr->indication_in_flight == false){

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

        else{
            bool failure = write_queue(htm_temperature_buffer, gattdb_temperature_measurement, 5);

            if(!failure)
              bleDataPtr->queued_indications++;
        }

      }
  }
}

void send_button_indication(uint8_t button_value)
{
  ble_data_struct_t *bleDataPtr = getBleDataPtr();

  uint8_t button_value_array[2];

  button_value_array[0] = 0;
  button_value_array[1] = button_value;

//    button_value_array[0] = button_value;
//    button_value_array[1] = 0;    A9 implementation

  if (bleDataPtr->connection_open == true)
    {

      sc = sl_bt_gatt_server_write_attribute_value(
          gattdb_button_state,
          0,
          1,
          (const uint8_t *)&button_value_array[0]);

      if (sc != SL_STATUS_OK) {
                   LOG_ERROR("sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x", (unsigned int) sc);
               }

      if (bleDataPtr->button_indication && bleDataPtr->bonded)
        {
          if (!bleDataPtr->indication_in_flight)
            {
              //no indication in-flight

              sc = sl_bt_gatt_server_send_indication(bleDataPtr->connection_handle,
                                                     gattdb_button_state,
                                                     2,
                                                     &button_value_array[0]);
              //LOG_INFO("sending button indication");
              if (sc != SL_STATUS_OK)
                      {
                           LOG_ERROR("sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x", (unsigned int) sc);
                      }
              else
                {
                  bleDataPtr->indication_in_flight = true;
                }
            }

          else
            {
              //indicaton in-flight
              bool failure = write_queue(button_value_array,
                                         gattdb_button_state,
                                         2);
              if(!failure)
                bleDataPtr->queued_indications++;
            }
        }
  }


}


#if !DEVICE_IS_BLE_SERVER

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

#endif


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

            sc = sl_bt_connection_set_default_parameters(60, 60, 4, 83, 0, 7);
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

          sc = sl_bt_sm_configure(0x0F, sl_bt_sm_io_capability_displayyesno);
          if (sc != SL_STATUS_OK) {
              LOG_ERROR("sl_bt_sm_configure() returned != 0 status=0x%04x", (unsigned int) sc);
          }

          sc = sl_bt_sm_delete_bondings();
          if (sc != SL_STATUS_OK) {
              LOG_ERROR("sl_bt_sm_delete_bondings() returned != 0 status=0x%04x", (unsigned int) sc);
          }

          displayPrintf(DISPLAY_ROW_BTADDR, "%02X:%02X:%02X:%02X:%02X:%02X",
                        bleDataPtr->myAddress.addr[0],
                        bleDataPtr->myAddress.addr[1],
                        bleDataPtr->myAddress.addr[2],
                        bleDataPtr->myAddress.addr[3],
                        bleDataPtr->myAddress.addr[4],
                        bleDataPtr->myAddress.addr[5]);
          displayPrintf(DISPLAY_ROW_ASSIGNMENT, "A9");

          // Initialising the flags
          bleDataPtr->connection_open = false;
          bleDataPtr->indication_in_flight = false;
          bleDataPtr->ok_to_send_htm_indications = false;
          bleDataPtr->bonded = false;
          bleDataPtr->button_indication = false;

      break;

    case sl_bt_evt_connection_opened_id:

      // handle open event
      bleDataPtr->connection_open = true;
      bleDataPtr->connection_handle = evt->data.evt_connection_opened.connection;
      bleDataPtr->queued_indications = 0;


#if DEVICE_IS_BLE_SERVER

          //SERVER
      sc = sl_bt_advertiser_stop((bleDataPtr->advertisingSetHandle));

      if (sc != SL_STATUS_OK) {
                    LOG_ERROR("sl_bt_advertiser_stop() returned != 0 status=0x%04x", (unsigned int) sc);
      }

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
       bleDataPtr->bonded = false;
       bleDataPtr->queued_indications = 0;
       bleDataPtr->indication_in_flight = false;

       displayPrintf(DISPLAY_ROW_9, "");
       displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
       displayPrintf(DISPLAY_ROW_PASSKEY, "");
       displayPrintf(DISPLAY_ROW_ACTION, "");
       displayPrintf(DISPLAY_ROW_BTADDR2, "");
       gpioLed1SetOff();
       gpioLed0SetOff();

       sc = sl_bt_sm_delete_bondings();
       if (sc != SL_STATUS_OK) {
           LOG_ERROR("sl_bt_sm_delete_bondings() returned != 0 status=0x%04x", (unsigned int) sc);
       }

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
       displayPrintf(DISPLAY_ROW_CONNECTION, "Scanning");

#endif

       break;

     case sl_bt_evt_connection_parameters_id:

//       LOG_INFO("Connection parameters: Connection=%d \n\r", (int)(evt->data.evt_connection_parameters.connection));
//       LOG_INFO("Interval=%d \n\r", (int)(evt->data.evt_connection_parameters.interval*1.25));
//       LOG_INFO("Latency=%d \n\r", (int)(evt->data.evt_connection_parameters.latency));
//       LOG_INFO("Timeout=%d \n\r", (int)(evt->data.evt_connection_parameters.timeout*10));

       break;

     case sl_bt_evt_system_external_signal_id:

//
//       typedef enum uint32_t {
//         PB0_pressed,
//         PB1_pressed,
//         PB1_released,
//         PB0_released
//       }toggle_states;

       if(evt->data.evt_system_external_signal.extsignals == evt_button_pressed)
         {

#if DEVICE_IS_BLE_SERVER

           displayPrintf(DISPLAY_ROW_9, "Button pressed");
           if(bleDataPtr->bonded)
             {
                 send_button_indication(0x01);
                 LOG_INFO("server- button pressed (0x01");
             }
#else
     //client
           if(bleDataPtr->pb1_button_status){
               sc = sl_bt_gatt_read_characteristic_value(bleDataPtr->connection_handle,
                                                         bleDataPtr->button_characteristic);
               if (sc != SL_STATUS_OK) {
                   LOG_ERROR("sl_bt_gatt_read_characteristic_value() returned != 0 status=0x%04x", (unsigned int) sc);
               }
           }
#endif

           if(bleDataPtr->button_status && bleDataPtr->bonded == false)
             {
               sc = sl_bt_sm_passkey_confirm(bleDataPtr->connection_handle, 1);

               if(sc != SL_STATUS_OK) {
                   LOG_ERROR("sl_bt_sm_passkey_confirm() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
               }
             }
         }

#if DEVICE_IS_BLE_SERVER
       else if(evt->data.evt_system_external_signal.extsignals == evt_button_released)
    {
           displayPrintf(DISPLAY_ROW_9, "Button released");

           if(bleDataPtr->bonded){
             send_button_indication(0x00);
             LOG_INFO("server- button released (0x00");
           }
    }
#endif

       break;

     case sl_bt_evt_system_soft_timer_id:
       displayUpdate();

#if DEVICE_IS_BLE_SERVER

       bool success;

       if (!bleDataPtr->indication_in_flight && bleDataPtr->queued_indications)
         {

           success = read_queue(&(dequeued_element.buffer[0]),
                                        &(dequeued_element.charHandle),
                                        &(dequeued_element.bufferLength));
//           LOG_INFO("Reading the values");
//           LOG_INFO("buffer length=%d \n\r", (int)(dequeued_element.bufferLength));
//           LOG_INFO("char handle=%d \n\r", (int)(dequeued_element.charHandle));
//           LOG_INFO("Buffer value: %d %d %d %d %d \n\r",  dequeued_element.buffer[4],
//               dequeued_element.buffer[3],
//               dequeued_element.buffer[2],
//               dequeued_element.buffer[1],
//               dequeued_element.buffer[0]);


           if (!success)
             {
               sc = sl_bt_gatt_server_send_indication(bleDataPtr->connection_handle,
                                               dequeued_element.charHandle,
                                               dequeued_element.bufferLength,
                                               &(dequeued_element.buffer[0]));

               if (sc != SL_STATUS_OK)
               {
                   LOG_ERROR("sl_bt_gatt_send_indication() returned != 0 status=0x%04x", (unsigned int) sc);
               }

               else
               {
                   bleDataPtr->indication_in_flight = true;
                   bleDataPtr->queued_indications--;
               }
             }

         }
#endif

       break;

     case sl_bt_evt_sm_confirm_bonding_id:

       sc = sl_bt_sm_bonding_confirm(bleDataPtr->connection_handle, 1);
       if (sc != SL_STATUS_OK) {
           LOG_ERROR("sl_bt_bonding_confirm() returned != 0 status=0x%04x", (unsigned int) sc);
       }

       break;

     case sl_bt_evt_sm_confirm_passkey_id:

       bleDataPtr->passkey = evt->data.evt_sm_confirm_passkey.passkey;

       displayPrintf(DISPLAY_ROW_PASSKEY, "%d", bleDataPtr->passkey);
       displayPrintf(DISPLAY_ROW_ACTION, "Confirm with PB0");
       break;

     case sl_bt_evt_sm_bonded_id:

       displayPrintf(DISPLAY_ROW_CONNECTION, "Bonded");
       displayPrintf(DISPLAY_ROW_PASSKEY, "");
       displayPrintf(DISPLAY_ROW_ACTION, "");

       bleDataPtr->bonded = true;

       break;

     case sl_bt_evt_sm_bonding_failed_id:

       bleDataPtr->bonded = false;

       //close connection
       sc = sl_bt_connection_close(bleDataPtr->connection_handle);
       if(sc != SL_STATUS_OK) {
           LOG_ERROR("sl_bt_connection_close() returned != 0 status=0x%04x\n\r", (unsigned int)sc);
       }
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
                   gpioLed0SetOff();
                 }

                 if (evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_server_indication){
                   bleDataPtr->ok_to_send_htm_indications = true;             // Client has enabled notifications
                   LOG_INFO("enabled htm indications");
                   gpioLed0SetOn();
                 }
             }

           if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_confirmation)
             {
               bleDataPtr->indication_in_flight = false;
             }

         }


       // Checking for the correct characteristic
       if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_button_state)
         {
           //LOG_INFO("in button state characterisitc");
           if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config)
             {
                 if (evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_server_disable){
                   bleDataPtr->button_indication = false;            // Notifications are disabled
                   gpioLed1SetOff();
                 }

                 if (evt->data.evt_gatt_server_characteristic_status.client_config_flags == sl_bt_gatt_server_indication){
                   //LOG_INFO("enabled button indications");
                   bleDataPtr->button_indication = true;             // Client has enabled notifications
                   gpioLed1SetOn();
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

       if(evt->data.evt_gatt_procedure_completed.result == 0x110F) {

           sc = sl_bt_sm_increase_security(bleDataPtr->connection_handle);
           if (sc != SL_STATUS_OK) {
               LOG_ERROR("sl_bt_sm_increase_security() returned != 0 status=0x%04x", (unsigned int) sc);
           }

       }
       break;

     case sl_bt_evt_gatt_service_id:

    //   bleDataPtr->service_handle = evt->data.evt_gatt_service.service;

       if (memcmp(evt->data.evt_gatt_service.uuid.data, service_uuid, sizeof(service_uuid)) == 0)
         {
           bleDataPtr->service_handle = evt->data.evt_gatt_service.service;
         }
       else if (memcmp(evt->data.evt_gatt_service.uuid.data, button_service_uuid, sizeof(button_service_uuid)) == 0)
         {
           bleDataPtr->button_service_handle = evt->data.evt_gatt_service.service;
         }

       break;


     case sl_bt_evt_gatt_characteristic_id:

//       bleDataPtr->characteristic = evt->data.evt_gatt_characteristic.characteristic;

       if (memcmp(evt->data.evt_gatt_characteristic.uuid.data, characteristic_uuid, sizeof(characteristic_uuid)) == 0)
         {
           bleDataPtr->characteristic = evt->data.evt_gatt_characteristic.characteristic;
         }
       else if (memcmp(evt->data.evt_gatt_characteristic.uuid.data, button_characteristic_uuid, sizeof(button_characteristic_uuid)) == 0)
         {
           bleDataPtr->button_characteristic = evt->data.evt_gatt_characteristic.characteristic;
         }
       break;


     case sl_bt_evt_gatt_characteristic_value_id:

       if(evt->data.evt_gatt_characteristic_value.att_opcode == sl_bt_gatt_handle_value_indication)
         {
           sc = sl_bt_gatt_send_characteristic_confirmation(bleDataPtr->connection_handle);
           if (sc != SL_STATUS_OK)
             {
               LOG_ERROR("sl_bt_gatt_send_characteristic_confirmation() returned != 0 status=0x%04x", (unsigned int) sc);
             }

         }
       if (evt->data.evt_gatt_characteristic_value.characteristic == bleDataPtr->characteristic)
         {
           //Receiving the temperature indications from the Server and displaying it on the Client’s LCD.
        bleDataPtr->characteristic_value = &(evt->data.evt_gatt_characteristic_value.value.data[0]);
        temperature_from_server = FLOAT_TO_INT32(bleDataPtr->characteristic_value);
        displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp = %d",temperature_from_server);
         }

       //if(bleDataPtr->bonded && bleDataPtr->button_indication && (evt->data.evt_gatt_characteristic_value.characteristic == bleDataPtr->button_characteristic)){
       if (evt->data.evt_gatt_characteristic_value.characteristic == bleDataPtr->button_characteristic)
         {
           LOG_INFO("Received button characteristic \n");

           if(evt->data.evt_gatt_characteristic_value.value.data[1] == 1) {
               LOG_INFO("Client- Button pressed on server \n");
               displayPrintf(DISPLAY_ROW_9, "Button Pressed");
           }
           else if(evt->data.evt_gatt_characteristic_value.value.data[1] == 0){
               LOG_INFO("Client- Button released on server \n");
               displayPrintf(DISPLAY_ROW_9, "Button Released");
           }
       }

       // for PB1 press
       if(evt->data.evt_gatt_characteristic_value.att_opcode == sl_bt_gatt_read_response) {
           LOG_INFO("PB1 press-read button value");
           if(evt->data.evt_gatt_characteristic_value.value.data[1] == 1) {
               displayPrintf(DISPLAY_ROW_9, "Button Pressed");
           }
           else if(evt->data.evt_gatt_characteristic_value.value.data[1] == 0){
               displayPrintf(DISPLAY_ROW_9, "Button Released");
           }
       }


       break;

#endif

  } // end - switch
} // handle_ble_event()
