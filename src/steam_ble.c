#include "steam_ble.h"
#include <string.h>

/* Original asynchronous client. UUIDs and volatile setup command format follow
 * the original Valve controller protocol documented by SDL and Bluepad32. */
const uint8_t bb_steam_service_uuid[16]={0x10,0x0f,0x6c,0x32,0x17,0x35,0x43,0x13,0xb4,0x02,0x38,0x56,0x71,0x31,0xe5,0xf3};
static const uint8_t input_uuid[16]={0x10,0x0f,0x6c,0x33,0x17,0x35,0x43,0x13,0xb4,0x02,0x38,0x56,0x71,0x31,0xe5,0xf3};
static const uint8_t feature_uuid[16]={0x10,0x0f,0x6c,0x34,0x17,0x35,0x43,0x13,0xb4,0x02,0x38,0x56,0x71,0x31,0xe5,0xf3};
static uint8_t clear_mappings[]={0xc0,0x81,0x00};
/* Default gyro off and disable built-in mouse emulation, expose both pads, disable absolute mouse smoothing.
 * No flash/firmware update commands are issued. */
static uint8_t configure_pads[]={0xc0,0x87,0x0c,0x30,0,0,0x07,7,0,0x08,7,0,0x18,0,0};
static bb_steam_ble_t* clients[2];
static void event_handler(uint8_t type,uint16_t channel,uint8_t* packet,uint16_t size);

const char* bb_steam_ble_stage_name(bb_steam_stage_t stage) {
    static const char* const names[]={"idle","service","characteristics","notifications","clear_mappings","configure_pads","ready","failed"};
    return (unsigned)stage<sizeof(names)/sizeof(names[0])?names[stage]:"invalid";
}
static void fail(bb_steam_ble_t* c,uint8_t status) {
    if(c->listening){gatt_client_stop_listening_for_characteristic_value_updates(&c->listener);c->listening=false;}
    c->failed_stage=c->stage;
    c->status=status;c->stage=BB_STEAM_FAILED;c->waiting=false;
    if(c->callback)c->callback(c,BB_STEAM_EVENT_FAILED,status,NULL,0);
}
void bb_steam_ble_stop(bb_steam_ble_t* c) {
    if(c->listening)gatt_client_stop_listening_for_characteristic_value_updates(&c->listener);
    if(c->slot<2 && clients[c->slot]==c)clients[c->slot]=NULL;
    memset(c,0,sizeof(*c));c->handle=HCI_CON_HANDLE_INVALID;
}
void bb_steam_ble_start(bb_steam_ble_t* c,uint8_t slot,hci_con_handle_t handle,uint32_t now,bool gyro,bb_steam_callback_t callback) {
    bb_steam_ble_stop(c);
    if(slot>=2||handle==HCI_CON_HANDLE_INVALID)return;
    c->slot=slot;c->handle=handle;c->callback=callback;c->started_at=c->step_at=c->next_action_at=now;
    memcpy(c->configure,configure_pads,sizeof(configure_pads));
    c->configure[4]=gyro?0x10:0; /* IMU mode: raw angular velocity only. */
    c->desired[2]=100;c->sent[2]=255;c->pending=4;
    c->stage=BB_STEAM_SERVICE;clients[slot]=c;
    bb_steam_ble_poll(c,now);
}
static void output_fail(bb_steam_ble_t* c,uint8_t status) {
    c->output_failed=true;c->output_status=status;++c->output_errors;
    c->pending=0; /* Optional feedback failure must never tear down input. */
}
static void queue_output(bb_steam_ble_t* c,unsigned kind,uint8_t value) {
    if(!c->pending)c->output_queued_at=btstack_run_loop_get_time_ms();
    c->desired[kind]=value;c->pending|=(uint8_t)(1u<<kind);
}
bool bb_steam_ble_haptics(bb_steam_ble_t* c,uint8_t left,uint8_t right) {
    if(c->stage!=BB_STEAM_READY||c->output_failed)return false;
    c->game_left=left;c->game_right=right;
    if(c->alert_active)return true; /* Keep the latest game effect for restoration. */
    /* Repeated nonzero requests renew a finite pulse train; zero is deduplicated. */
    if(left||c->desired[0]||c->sent[0])queue_output(c,0,left);
    if(right||c->desired[1]||c->sent[1])queue_output(c,1,right);
    return true;
}
bool bb_steam_ble_alert(bb_steam_ble_t* c,bool active,uint8_t left,uint8_t right,bool refresh) {
    if(c->stage!=BB_STEAM_READY||c->output_failed)return false;
    bool changed=c->alert_active!=active;c->alert_active=active;
    if(!active) {
        if(changed) {
            queue_output(c,0,c->game_left);queue_output(c,1,c->game_right);
        }
        return true;
    }
    if(changed || c->desired[0]!=left || (refresh&&left))queue_output(c,0,left);
    if(changed || c->desired[1]!=right || (refresh&&right))queue_output(c,1,right);
    return true;
}
bool bb_steam_ble_led(bb_steam_ble_t* c,uint8_t brightness) {
    if(c->stage!=BB_STEAM_READY||c->output_failed||brightness>100)return false;
    if(c->desired[2]!=brightness||c->sent[2]!=brightness)queue_output(c,2,brightness);
    return true;
}
static void poll_output(bb_steam_ble_t* c,uint32_t now) {
    if(c->output_failed)return;
    if(c->waiting) {
        if((uint32_t)(now-c->output_at)>=3000u)output_fail(c,ERROR_CODE_CONNECTION_TIMEOUT);
        /* On timeout retain buffer and waiting flag until completion/disconnect. */
        return;
    }
    if(!c->pending)return;
    if((uint32_t)(now-c->output_queued_at)>=3000u){output_fail(c,ERROR_CODE_CONNECTION_TIMEOUT);return;}
    if((int32_t)(now-c->output_retry_at)<0)return;
    unsigned kind=3;
    for(unsigned i=0;i<2;i++)if((c->pending&(1u<<i))&&!c->desired[i]){kind=i;break;}
    if(kind==3)for(unsigned i=0;i<3;i++) {
        unsigned k=(c->cursor+i)%3u;if(c->pending&(1u<<k)){kind=k;break;}
    }
    if(kind==3)return;
    uint8_t value=c->desired[kind],length;
    memset(c->output,0,sizeof(c->output));c->output[0]=0xc0;
    if(kind==2) {
        /* SET_SETTINGS: white Steam-logo LED user brightness (0..100). */
        c->output[1]=0x87;c->output[2]=3;c->output[3]=0x2d;c->output[4]=value;length=6;
    } else {
        /* Original-controller pulse command: duration/interval in microseconds,
         * count finite, gain 0 dB. Legacy wire pad numbers are left=1/right=0. */
        uint16_t duration=(uint16_t)((unsigned)value*500u/255u);
        c->output[1]=0x8f;c->output[2]=8;c->output[3]=(uint8_t)(kind^1u);
        c->output[4]=(uint8_t)duration;c->output[5]=(uint8_t)(duration>>8);
        c->output[6]=0xa0;c->output[7]=0x0f;c->output[8]=value?25:0;length=11;
    }
    uint8_t status=gatt_client_write_value_of_characteristic(event_handler,c->handle,c->feature.value_handle,length,c->output);
    if(status==ERROR_CODE_SUCCESS) {
        c->pending&=(uint8_t)~(1u<<kind);c->waiting=true;c->output_at=now;
        c->output_kind=(uint8_t)kind;c->output_value=value;c->cursor=(uint8_t)((kind+1u)%3u);
        if(c->pending)c->output_queued_at=now;
    } else if(status==GATT_CLIENT_IN_WRONG_STATE||status==BTSTACK_MEMORY_ALLOC_FAILED)c->output_retry_at=now+10u;
    else output_fail(c,status);
}
void bb_steam_ble_poll(bb_steam_ble_t* c,uint32_t now) {
    if(c->stage==BB_STEAM_READY){poll_output(c,now);return;}
    if(c->stage==BB_STEAM_IDLE||c->stage>=BB_STEAM_READY)return;
    if((uint32_t)(now-c->started_at)>=10000u||(uint32_t)(now-c->step_at)>=3000u){fail(c,ERROR_CODE_CONNECTION_TIMEOUT);return;}
    if(c->waiting||(int32_t)(now-c->next_action_at)<0)return;
    uint8_t status;
    c->waiting=true;
    switch(c->stage) {
        case BB_STEAM_SERVICE:
            status=gatt_client_discover_primary_services_by_uuid128(event_handler,c->handle,bb_steam_service_uuid);break;
        case BB_STEAM_CHARACTERISTICS:
            status=gatt_client_discover_characteristics_for_service(event_handler,c->handle,&c->service);break;
        case BB_STEAM_NOTIFY:
            if(!c->listening) {
                gatt_client_listen_for_characteristic_value_updates(&c->listener,event_handler,c->handle,&c->input);
                c->listening=true;
            }
            status=gatt_client_write_client_characteristic_configuration(event_handler,c->handle,&c->input,GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);break;
        case BB_STEAM_CLEAR:
            status=gatt_client_write_value_of_characteristic(event_handler,c->handle,c->feature.value_handle,sizeof(clear_mappings),clear_mappings);break;
        case BB_STEAM_CONFIG:
            status=gatt_client_write_value_of_characteristic(event_handler,c->handle,c->feature.value_handle,sizeof(c->configure),c->configure);break;
        default:c->waiting=false;return;
    }
    if(status==ERROR_CODE_SUCCESS)return;
    c->waiting=false;
    if(status!=GATT_CLIENT_IN_WRONG_STATE && status!=BTSTACK_MEMORY_ALLOC_FAILED)fail(c,status);
}
static void event_handler(uint8_t type,uint16_t channel,uint8_t* p,uint16_t size) {
    (void)channel;
    if(type!=HCI_EVENT_PACKET||size<4||(unsigned)p[1]+2u!=size)return;
    uint16_t handle=little_endian_read_16(p,2);
    bb_steam_ble_t* c=NULL;
    for(unsigned i=0;i<2;i++)if(clients[i]&&clients[i]->handle==handle)c=clients[i];
    if(!c||c->stage==BB_STEAM_IDLE||c->stage==BB_STEAM_FAILED)return;
    if(p[0]==GATT_EVENT_NOTIFICATION) {
        if(size<12||c->stage!=BB_STEAM_READY||gatt_event_notification_get_value_handle(p)!=c->input.value_handle)return;
        uint16_t length=gatt_event_notification_get_value_length(p);
        if(length>size-12u)return;
        ++c->notifications;
        if(c->callback)c->callback(c,BB_STEAM_EVENT_INPUT,0,gatt_event_notification_get_value(p),length);
        return;
    }
    if(!c->waiting)return;
    if(p[0]==GATT_EVENT_SERVICE_QUERY_RESULT&&c->stage==BB_STEAM_SERVICE) {
        if(size<28)return;
        gatt_client_service_t service;gatt_event_service_query_result_get_service(p,&service);
        if(memcmp(service.uuid128,bb_steam_service_uuid,16))return;
        if(!service.start_group_handle||service.end_group_handle<service.start_group_handle){fail(c,ATT_ERROR_ATTRIBUTE_NOT_FOUND);return;}
        if(c->service.start_group_handle && c->service.start_group_handle!=service.start_group_handle){fail(c,ATT_ERROR_ATTRIBUTE_NOT_FOUND);return;}
        c->service=service;return;
    }
    if(p[0]==GATT_EVENT_CHARACTERISTIC_QUERY_RESULT&&c->stage==BB_STEAM_CHARACTERISTICS) {
        if(size<32)return;
        gatt_client_characteristic_t characteristic;gatt_event_characteristic_query_result_get_characteristic(p,&characteristic);
        if(characteristic.start_handle<c->service.start_group_handle || characteristic.end_handle>c->service.end_group_handle ||
           characteristic.value_handle<=characteristic.start_handle || characteristic.value_handle>characteristic.end_handle)return;
        if(!memcmp(characteristic.uuid128,input_uuid,16))c->input=characteristic;
        if(!memcmp(characteristic.uuid128,feature_uuid,16))c->feature=characteristic;
        return;
    }
    if(p[0]!=GATT_EVENT_QUERY_COMPLETE||size<9)return;
    uint8_t status=gatt_event_query_complete_get_att_status(p);
    if(c->stage==BB_STEAM_READY) {
        c->waiting=false;
        if(status)output_fail(c,status);
        else {c->sent[c->output_kind]=c->output_value;++c->output_writes;}
        return;
    }
    if(status){fail(c,status);return;}
    if((c->stage==BB_STEAM_SERVICE&&!c->service.start_group_handle)||
       (c->stage==BB_STEAM_CHARACTERISTICS&&(!c->input.value_handle||!c->feature.value_handle||
       !(c->input.properties&ATT_PROPERTY_NOTIFY)||!(c->feature.properties&ATT_PROPERTY_WRITE)))) {
        fail(c,ATT_ERROR_ATTRIBUTE_NOT_FOUND);return;
    }
    c->waiting=false;c->stage=(bb_steam_stage_t)(c->stage+1);c->step_at=btstack_run_loop_get_time_ms();
    /* Allow controller-side clear processing without sleeping in the BT loop. */
    c->next_action_at=c->step_at+(c->stage==BB_STEAM_CONFIG?3u:0u);
    if(c->stage==BB_STEAM_READY){
        c->output_queued_at=c->output_retry_at=c->step_at;
        if(c->callback)c->callback(c,BB_STEAM_EVENT_READY,0,NULL,0);
    }
    else bb_steam_ble_poll(c,c->step_at);
}
