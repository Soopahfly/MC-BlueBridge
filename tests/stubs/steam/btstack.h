#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define HCI_EVENT_PACKET 4
#define HCI_CON_HANDLE_INVALID 0xffff
#define ERROR_CODE_SUCCESS 0
#define ERROR_CODE_CONNECTION_TIMEOUT 8
#define GATT_CLIENT_IN_WRONG_STATE 0x95
#define BTSTACK_MEMORY_ALLOC_FAILED 0x56
#define ATT_ERROR_ATTRIBUTE_NOT_FOUND 0x0a
#define ATT_PROPERTY_NOTIFY 0x10
#define ATT_PROPERTY_WRITE 8
#define GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION 1
#define GATT_EVENT_QUERY_COMPLETE 0xa0
#define GATT_EVENT_SERVICE_QUERY_RESULT 0xa1
#define GATT_EVENT_CHARACTERISTIC_QUERY_RESULT 0xa2
#define GATT_EVENT_NOTIFICATION 0xa7
typedef uint16_t hci_con_handle_t;
typedef void (*btstack_packet_handler_t)(uint8_t,uint16_t,uint8_t*,uint16_t);
typedef struct {uint16_t start_group_handle,end_group_handle,uuid16;uint8_t uuid128[16];} gatt_client_service_t;
typedef struct {uint16_t start_handle,value_handle,end_handle,properties,uuid16;uint8_t uuid128[16];} gatt_client_characteristic_t;
typedef struct {btstack_packet_handler_t callback;bool active;} gatt_client_notification_t;
static inline uint16_t little_endian_read_16(const uint8_t* p,unsigned at){return (uint16_t)(p[at]|(p[at+1]<<8));}
static inline void gatt_event_service_query_result_get_service(const uint8_t* p,gatt_client_service_t* s){
    memset(s,0,sizeof(*s));s->start_group_handle=little_endian_read_16(p,8);s->end_group_handle=little_endian_read_16(p,10);
    for(unsigned i=0;i<16;i++)s->uuid128[i]=p[12+15-i];
}
static inline void gatt_event_characteristic_query_result_get_characteristic(const uint8_t* p,gatt_client_characteristic_t* c){
    memset(c,0,sizeof(*c));c->start_handle=little_endian_read_16(p,8);c->value_handle=little_endian_read_16(p,10);
    c->end_handle=little_endian_read_16(p,12);c->properties=little_endian_read_16(p,14);
    for(unsigned i=0;i<16;i++)c->uuid128[i]=p[16+15-i];
}
static inline uint8_t gatt_event_query_complete_get_att_status(const uint8_t* p){return p[8];}
static inline uint16_t gatt_event_notification_get_value_handle(const uint8_t* p){return little_endian_read_16(p,8);}
static inline uint16_t gatt_event_notification_get_value_length(const uint8_t* p){return little_endian_read_16(p,10);}
static inline const uint8_t* gatt_event_notification_get_value(const uint8_t* p){return p+12;}
uint32_t btstack_run_loop_get_time_ms(void);
uint8_t gatt_client_discover_primary_services_by_uuid128(btstack_packet_handler_t,hci_con_handle_t,const uint8_t*);
uint8_t gatt_client_discover_characteristics_for_service(btstack_packet_handler_t,hci_con_handle_t,gatt_client_service_t*);
void gatt_client_listen_for_characteristic_value_updates(gatt_client_notification_t*,btstack_packet_handler_t,hci_con_handle_t,gatt_client_characteristic_t*);
void gatt_client_stop_listening_for_characteristic_value_updates(gatt_client_notification_t*);
uint8_t gatt_client_write_client_characteristic_configuration(btstack_packet_handler_t,hci_con_handle_t,gatt_client_characteristic_t*,uint16_t);
uint8_t gatt_client_write_value_of_characteristic(btstack_packet_handler_t,hci_con_handle_t,uint16_t,uint16_t,uint8_t*);
