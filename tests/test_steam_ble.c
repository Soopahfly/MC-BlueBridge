#include "steam_ble.h"
#include <assert.h>
#include <stdio.h>

static uint32_t now;
static uint8_t next_status;
static unsigned operations,ready[2],failed[2],inputs[2],stopped;
static uint8_t command[20];static uint16_t command_len;
static btstack_packet_handler_t handler[2];
static bb_steam_ble_t *test_a,*test_b;
uint32_t btstack_run_loop_get_time_ms(void){return now;}
static uint8_t operation(btstack_packet_handler_t cb,hci_con_handle_t handle){
    assert(handle==0x40||handle==0x41);handler[handle-0x40]=cb;++operations;
    uint8_t status=next_status;next_status=0;return status;
}
uint8_t gatt_client_discover_primary_services_by_uuid128(btstack_packet_handler_t cb,hci_con_handle_t h,const uint8_t* uuid){
    assert(!memcmp(uuid,bb_steam_service_uuid,16));return operation(cb,h);
}
uint8_t gatt_client_discover_characteristics_for_service(btstack_packet_handler_t cb,hci_con_handle_t h,gatt_client_service_t* s){
    assert(s->start_group_handle==1&&s->end_group_handle==32);return operation(cb,h);
}
void gatt_client_listen_for_characteristic_value_updates(gatt_client_notification_t* n,btstack_packet_handler_t cb,hci_con_handle_t h,gatt_client_characteristic_t* c){
    (void)h;assert(c->value_handle==5);n->callback=cb;n->active=true;
}
void gatt_client_stop_listening_for_characteristic_value_updates(gatt_client_notification_t* n){assert(n->active);n->active=false;++stopped;}
uint8_t gatt_client_write_client_characteristic_configuration(btstack_packet_handler_t cb,hci_con_handle_t h,gatt_client_characteristic_t* c,uint16_t config){
    assert(c->value_handle==5&&config==1);return operation(cb,h);
}
uint8_t gatt_client_write_value_of_characteristic(btstack_packet_handler_t cb,hci_con_handle_t h,uint16_t handle,uint16_t length,uint8_t* value){
    assert(handle==9&&length<=sizeof(command));command_len=length;memcpy(command,value,length);return operation(cb,h);
}
static void callback(bb_steam_ble_t* c,bb_steam_event_t event,uint8_t status,const uint8_t* p,uint16_t length){
    if(event==BB_STEAM_EVENT_READY)++ready[c->slot];
    else if(event==BB_STEAM_EVENT_FAILED){assert(status);++failed[c->slot];}
    else {assert(length==19&&p[0]==0xc0);++inputs[c->slot];}
}
static void header(uint8_t* p,unsigned length,uint8_t type,uint16_t handle){memset(p,0,length);p[0]=type;p[1]=(uint8_t)(length-2);p[2]=(uint8_t)handle;p[3]=(uint8_t)(handle>>8);}
static void complete(uint16_t h,uint8_t status){uint8_t p[9];header(p,9,GATT_EVENT_QUERY_COMPLETE,h);p[8]=status;handler[h-0x40](HCI_EVENT_PACKET,0,p,9);}
static void service(uint16_t h){uint8_t p[28];header(p,28,GATT_EVENT_SERVICE_QUERY_RESULT,h);p[8]=1;p[10]=32;for(unsigned i=0;i<16;i++)p[12+i]=bb_steam_service_uuid[15-i];handler[h-0x40](HCI_EVENT_PACKET,0,p,28);}
static void characteristic(uint16_t h,bool input){uint8_t p[32];header(p,32,GATT_EVENT_CHARACTERISTIC_QUERY_RESULT,h);
    p[8]=input?4:8;p[10]=input?5:9;p[12]=input?7:11;p[14]=input?ATT_PROPERTY_NOTIFY:ATT_PROPERTY_WRITE;
    uint8_t uuid[16];memcpy(uuid,bb_steam_service_uuid,16);uuid[3]=input?0x33:0x34;
    for(unsigned i=0;i<16;i++)p[16+i]=uuid[15-i];
    handler[h-0x40](HCI_EVENT_PACKET,0,p,32);
}
static void discover(uint16_t h){service(h);complete(h,0);characteristic(h,true);characteristic(h,false);complete(h,0);}
static void finish(uint16_t h){
    complete(h,0);assert(command_len==3&&command[0]==0xc0&&command[1]==0x81&&command[2]==0);
    complete(h,0);now+=3;bb_steam_ble_poll(h==0x40?test_a:test_b,now);assert(command_len==15&&command[1]==0x87&&command[2]==12);
    complete(h,0);
}
int main(void){
    bb_steam_ble_t a={0},b={0};test_a=&a;test_b=&b;
    bb_steam_ble_start(&a,0,0x40,now,false,callback);assert(a.stage==BB_STEAM_SERVICE&&a.waiting);
    discover(0x40);assert(a.stage==BB_STEAM_NOTIFY&&!ready[0]);finish(0x40);assert(ready[0]==1&&a.stage==BB_STEAM_READY);
    uint8_t p[31];header(p,31,GATT_EVENT_NOTIFICATION,0x40);p[8]=5;p[10]=19;p[12]=0xc0;
    handler[0](HCI_EVENT_PACKET,0,p,sizeof(p));assert(inputs[0]==1);
    p[10]=20;handler[0](HCI_EVENT_PACKET,0,p,sizeof(p));assert(inputs[0]==1); /* Truncated notification. */
    bb_steam_ble_start(&b,1,0x41,now,true,callback);discover(0x41);finish(0x41);assert(ready[1]==1&&command[4]==0x10&&a.configure[4]==0);
    bb_steam_ble_stop(&a);p[10]=19;handler[0](HCI_EVENT_PACKET,0,p,sizeof(p));assert(inputs[0]==1&&b.stage==BB_STEAM_READY);
    bb_steam_ble_stop(&b);assert(stopped==2);
    /* No service and a GATT rejection must fail rather than claim ready. */
    bb_steam_ble_start(&a,0,0x40,now,false,callback);complete(0x40,0);assert(failed[0]==1&&a.failed_stage==BB_STEAM_SERVICE);
    bb_steam_ble_stop(&a);bb_steam_ble_start(&a,0,0x40,now,false,callback);discover(0x40);complete(0x40,5);
    assert(failed[0]==2&&!a.listening&&a.failed_stage==BB_STEAM_NOTIFY);bb_steam_ble_stop(&a);
    /* Busy request retries only when polled, and waiting requests time out. */
    next_status=GATT_CLIENT_IN_WRONG_STATE;bb_steam_ble_start(&a,0,0x40,now,false,callback);assert(!a.waiting);
    unsigned before=operations;now++;bb_steam_ble_poll(&a,now);assert(a.waiting&&operations==before+1);
    now+=3000;bb_steam_ble_poll(&a,now);assert(failed[0]==3&&a.status==ERROR_CODE_CONNECTION_TIMEOUT);
    bb_steam_ble_stop(&a);
    /* Newer Steam variants without the original input characteristic are rejected. */
    bb_steam_ble_start(&a,0,0x40,now,false,callback);service(0x40);complete(0x40,0);characteristic(0x40,false);complete(0x40,0);
    assert(a.stage==BB_STEAM_FAILED&&failed[0]==4);bb_steam_ble_stop(&a);
    now=0xfffffff0u;bb_steam_ble_start(&a,0,0x40,now,false,callback);
    now=3010;bb_steam_ble_poll(&a,now);assert(a.stage==BB_STEAM_FAILED);bb_steam_ble_stop(&a);
    puts("Steam BLE: discovery/setup order, ready gating, malformed input, two clients, cleanup, failures and timeout passed");
}
