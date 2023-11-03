/* @file      scheduler.c
 * @version   1.0
 * @brief     Functions to maintain scheduling of events
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
 *
 * @resources  -
 */

#include "src/i2c.h"
#include "src/scheduler.h"

#include "src/gpio.h"
#include "src/timers.h"
#include "ble.h"
#include "sl_bt_api.h"
#include "src/ble_device_type.h"
#include "sl_bluetooth.h"
#include "src/lcd.h"


#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"


#define POWERUP_TIME      (80000)
#define CONVERTION_TIME    (10800)


#define   MY_NUM_STATES (5)
typedef enum uint32_t {
  STATE0_IDLE,                      // state for POR sequence
  STATE1_TIMER_WAIT,                // state for writing I2C command
  STATE2_WARMUP,                    // state for conversion time delay
  STATE3_MEASUREMENT,               // state for reading I2C command
  STATE4_REPORT,                    // state to print temperature values
  STATE0,
  STATE1,
  STATE2,
  STATE3,
  STATE4,
  STATE5,
  STATE6,
  STATE7,
}my_states;



uint32_t myEvents = 0;          // variable to hold all events
sl_status_t rc = 0;

static const uint8_t characteristic_uuid[] =  { 0x1c, 0x2a }; // Reverse the byte order for little-endian format
static const uint8_t service_uuid[2] = { 0x09, 0x18 };

static const uint8_t button_characteristic_uuid[] =  { 0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x02, 0x00, 0x00, 0x00 }; // Reverse the byte order for little-endian format
static const uint8_t button_service_uuid[] =  { 0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x01, 0x00, 0x00, 0x00 };





void schedulerSetEventUF()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();            // enter critical, turn off interrupts in NVIC
  sl_bt_external_signal(evtUF_LETIMER0);
  CORE_EXIT_CRITICAL();             // exit critical, re-enable interrupts in NVIC
}

void schedulerSetEventCOMP1()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();            // enter critical, turn off interrupts in NVIC
  sl_bt_external_signal(evtCOMP1_LETIMER0);    // RMW
  CORE_EXIT_CRITICAL();             // exit critical, re-enable interrupts in NVIC
}

void schedulerSetEventI2CTransfer()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();                    // enter critical, turn off interrupts in NVIC
  sl_bt_external_signal(evt_I2CTransferComplete);     // RMW
  CORE_EXIT_CRITICAL();                     // exit critical, re-enable interrupts in NVIC
}

void schedulerSetEventButtonReleased()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();                    // enter critical, turn off interrupts in NVIC
  sl_bt_external_signal(evt_button_released);     // RMW
  CORE_EXIT_CRITICAL();
}

void schedulerSetEventButtonPressed()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();                    // enter critical, turn off interrupts in NVIC
  sl_bt_external_signal(evt_button_pressed);     // RMW
  CORE_EXIT_CRITICAL();
}

void state_machine(sl_bt_msg_t *evt)
{
  static my_states nextState = STATE0_IDLE;               /* Attribution: Instructor Dave Sluiter*/
         my_states state;

  ble_data_struct_t *bleDataPtr = getBleDataPtr();

  /* Attribution: Instructor Dave Sluiter*/
  // On each call, transfer nextState to state, this is similar to how
  // a hardware state machine transfers nextState to the state flip-flops
  // on each rising edge of the clock signal
  state = nextState;

  /* Attribution: Instructor Dave Sluiter*/
  //if((SL_BT_MSG_ID(evt->header) == sl_bt_evt_system_external_signal_id)){
  // Return if:
  //   a) not an sl_bt_evt_system_external_signal_id event, or
  //   b) client has not enabled indications for this characteristic
  //   c) not in an open connection
  if( (SL_BT_MSG_ID(evt->header) != sl_bt_evt_system_external_signal_id) ||
      (bleDataPtr->ok_to_send_htm_indications != true) ||
      (bleDataPtr->connection_open != true) ) {

     return;

  } // if

  switch (state)
  {
    case STATE0_IDLE:
      nextState = STATE0_IDLE;      //default
      if (evt->data.evt_system_external_signal.extsignals == evtUF_LETIMER0)
        {
          //sensor_enable();                          // power up the device
          timerWaitUs_interrupt(POWERUP_TIME);      // interrupt for powerup time
          nextState = STATE1_TIMER_WAIT;
        }
      break;

    case STATE1_TIMER_WAIT:
      nextState = STATE1_TIMER_WAIT;      //default
      if (evt->data.evt_system_external_signal.extsignals == evtCOMP1_LETIMER0)
        {
          sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);    // Power requirement for I2C
          i2c_write();                               // I2C write command
          nextState = STATE2_WARMUP;
        }
      break;

    case STATE2_WARMUP:
      nextState = STATE2_WARMUP;      //default
      if (evt->data.evt_system_external_signal.extsignals == evt_I2CTransferComplete)
        {
          sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
          timerWaitUs_interrupt(CONVERTION_TIME);           // interrupt for powerup time
          nextState = STATE3_MEASUREMENT;
        }
      break;

    case STATE3_MEASUREMENT:
      nextState = STATE3_MEASUREMENT;      //default
      if (evt->data.evt_system_external_signal.extsignals == evtCOMP1_LETIMER0)
        {
          sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
          i2c_read();
          nextState = STATE4_REPORT;
        }
      break;

    case STATE4_REPORT:
      nextState = STATE4_REPORT;      //default
      if (evt->data.evt_system_external_signal.extsignals == evt_I2CTransferComplete)
        {
          sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
          //sensor_disable();
          NVIC_DisableIRQ(I2C0_IRQn);
          send_temperature();
          nextState = STATE0_IDLE;
        }
      break;

    default:

      break;
  }

}

void discovery_state_machine(sl_bt_msg_t *evt)
{
  static my_states nextState = STATE0;               /* Attribution: Instructor Dave Sluiter*/
          my_states state;

  ble_data_struct_t *bleDataPtr = getBleDataPtr();

  //default state
  if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_connection_closed_id) {
      nextState = STATE0;
  }

  state = nextState;

  switch (state)
  {
    case STATE0:
      nextState = STATE0;      //default

      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_connection_opened_id)
          {
          // 1-time discovery of the HTM service using its service UUID
              rc = sl_bt_gatt_discover_primary_services_by_uuid(bleDataPtr->connection_handle,
                                              sizeof(service_uuid),
                                              (const uint8_t *)service_uuid);

              if (rc != SL_STATUS_OK) {
                           LOG_ERROR("sl_bt_gatt_discover_primary_services_by_uuid() returned != 0 status=0x%04x", (unsigned int) rc);
                       }
              nextState = STATE1;
          }
      break;

    case STATE1:
      nextState = STATE1;



      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
        {
          rc = sl_bt_gatt_discover_characteristics_by_uuid(bleDataPtr->connection_handle,
                                                           bleDataPtr->service_handle,
                                                           sizeof(characteristic_uuid),
                                                           (const uint8_t *)characteristic_uuid);

          if (rc != SL_STATUS_OK) {
                       LOG_ERROR("sl_bt_gatt_discover_characteristics_by_uuid() returned != 0 status=0x%04x", (unsigned int) rc);
                   }

          nextState = STATE2;
        }
      break;

    case STATE2:
      nextState = STATE2;
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
        {
          rc = sl_bt_gatt_set_characteristic_notification(bleDataPtr->connection_handle,
                                                          bleDataPtr->characteristic,
                                                          sl_bt_gatt_indication);

          if (rc != SL_STATUS_OK) {
                       LOG_ERROR("sl_bt_gatt_set_characteristic_notification() returned != 0 status=0x%04x", (unsigned int) rc);
                   }

          displayPrintf(DISPLAY_ROW_CONNECTION, "Handling Indications");

          nextState = STATE3;
        }
      break;

    case STATE3:
      nextState = STATE3;
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
        {
          rc = sl_bt_gatt_discover_primary_services_by_uuid(bleDataPtr->connection_handle,
                                                     sizeof(button_service_uuid),
                                                     (const uint8_t *)button_service_uuid);

          if (rc != SL_STATUS_OK) {
                                  LOG_ERROR("sl_bt_gatt_discover_primary_services_by_uuid() returned != 0 status=0x%04x", (unsigned int) rc);
          }

          nextState = STATE4;
        }
     break;

    case STATE4:
      nextState = STATE4;
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
        {
          rc = sl_bt_gatt_discover_characteristics_by_uuid(bleDataPtr->connection_handle,
                                                           bleDataPtr->button_service_handle,
                                                           sizeof(button_characteristic_uuid),
                                                           (const uint8_t *)button_characteristic_uuid);

          if (rc != SL_STATUS_OK) {
                       LOG_ERROR("sl_bt_gatt_discover_characteristics_by_uuid() returned != 0 status=0x%04x", (unsigned int) rc);
                   }
          nextState = STATE5;
        }
     break;


    case STATE5:
      nextState = STATE5;
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
        {
          rc = sl_bt_gatt_set_characteristic_notification(bleDataPtr->connection_handle,
                                                          bleDataPtr->button_characteristic,
                                                          sl_bt_gatt_indication);

          if (rc != SL_STATUS_OK) {
                       LOG_ERROR("sl_bt_gatt_set_characteristic_notification() returned != 0 status=0x%04x", (unsigned int) rc);
                   }
          nextState = STATE6;
        }
     break;

    case STATE6:
      nextState = STATE6;
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
        {
          nextState = STATE7;
        }
     break;

    case STATE7:
      nextState = STATE7;
      if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_connection_closed_id)
        {
          nextState = STATE0;
        }
     break;

    default:

      break;

  }
}

//if(SL_BT_MSG_ID(evt->header) == sl_bt_evt_connection_closed_id)
