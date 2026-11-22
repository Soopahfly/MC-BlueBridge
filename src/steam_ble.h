#pragma once
#include "btstack.h"

typedef enum { BB_STEAM_IDLE,BB_STEAM_SERVICE,BB_STEAM_CHARACTERISTICS,
    BB_STEAM_NOTIFY,BB_STEAM_CLEAR,BB_STEAM_CONFIG,BB_STEAM_READY,BB_STEAM_FAILED } bb_steam_stage_t;
typedef enum { BB_STEAM_EVENT_READY,BB_STEAM_EVENT_INPUT,BB_STEAM_EVENT_FAILED } bb_steam_event_t;
struct bb_steam_ble;
typedef void (*bb_steam_callback_t)(struct bb_steam_ble*,bb_steam_event_t,uint8_t,const uint8_t*,uint16_t);
typedef struct bb_steam_ble {
    gatt_client_service_t service;
    gatt_client_characteristic_t input,feature;
    gatt_client_notification_t listener;
    bb_steam_callback_t callback;
    hci_con_handle_t handle;
    uint32_t started_at,step_at,next_action_at,notifications;
    uint8_t slot,status;
    bb_steam_stage_t stage,failed_stage;
    bool waiting,listening;
    uint8_t configure[15];
    /* Owned until QUERY_COMPLETE: BTstack may retain the write buffer. */
    uint8_t output[11],desired[3],sent[3],pending,output_kind,output_value,cursor;
    uint8_t output_status;
    bool output_failed;
    uint32_t output_at,output_queued_at,output_retry_at,output_writes,output_errors;
} bb_steam_ble_t;

extern const uint8_t bb_steam_service_uuid[16];
void bb_steam_ble_start(bb_steam_ble_t* client,uint8_t slot,hci_con_handle_t handle,
    uint32_t now,bool gyro,bb_steam_callback_t callback);
void bb_steam_ble_poll(bb_steam_ble_t* client,uint32_t now);
bool bb_steam_ble_haptics(bb_steam_ble_t* client,uint8_t left,uint8_t right);
bool bb_steam_ble_led(bb_steam_ble_t* client,uint8_t brightness);
void bb_steam_ble_stop(bb_steam_ble_t* client);
const char* bb_steam_ble_stage_name(bb_steam_stage_t stage);
