#define BB_USE_ENCODER 0
#define BB_USE_TRACKBALL 1
#include "trackball.h"
static bool fake_ready;
static bb_trackball_event_t fake_event;
static unsigned fake_polls,fake_inits;
void bb_trackball_init(void){++fake_inits;}
void bb_trackball_poll(uint32_t now){(void)now;++fake_polls;}
bool bb_trackball_available(void){return fake_ready;}
void bb_trackball_take(bb_trackball_event_t* e){*e=fake_event;fake_event.x=fake_event.y=0;fake_event.clicks=0;}
void bb_trackball_colour(uint8_t r,uint8_t g,uint8_t b,uint8_t w){(void)r;(void)g;(void)b;(void)w;}
#define main common_panel_checks
#include "test_panel.c"
#undef main
int main(void){
    assert(common_panel_checks()==0);
    assert(bb_config_reset());bb_config_flush_pending();bb_state_init();
    assert(bb_panel_input_set(BB_PANEL_TRACKBALL));bb_config_flush_pending();bb_state_init();
    bb_panel_init();assert(fake_inits==1);
    uint8_t addr[6]={2,4,6,8,10,12},index;
    assert(bb_controller_activate_or_create(addr,1,2,"Trackball test pad",&index));
    bb_state_set_controller(true,index,"Trackball test pad");
    menu=editing=false;fake_ready=true;fake_event=(bb_trackball_event_t){0};snapshot();
    fake_event.y=1;tick();assert(menu&&item==USB_MODE); /* Roll opens menu. */
    fake_event.y=3;tick();assert(item==TURBO);
    fake_event.clicks=1;tick();assert(editing&&value==0); /* Fast click selects. */
    fake_event.y=1;tick();assert(value==0); /* Vertical movement cannot edit. */
    fake_event.x=1;tick();assert(value==1);
    fake_event.clicks=1;tick();assert(!editing&&profile()->turbo_enabled);
    /* A normal held click goes back once; release cannot Apply afterwards. */
    fake_event.down=true;tick();test_time+=40;tick();
    test_time+=800;tick();assert(!menu);
    fake_event.down=false;tick();test_time+=40;tick();assert(!menu);
    menu=true;item=TURBO;begin_edit();assert(editing);
    fake_event.down=true;tick();test_time+=40;tick();
    fake_ready=false;tick();test_time+=900;tick();assert(editing); /* Device loss does not synthesize click/hold. */
    fake_ready=true;fake_event=(bb_trackball_event_t){0};tick();assert(editing);
    assert(bb_panel_input_set(BB_PANEL_NONE));bb_config_flush_pending();bb_state_init();
    bb_panel_init();assert(fake_inits==1); /* None never initializes trackball. */
    menu=editing=false;fake_event.y=3;fake_event.clicks=1;
    unsigned polls=fake_polls;tick();
    assert(!menu&&!editing&&fake_polls==polls); /* None ignores all physical events. */
    puts("trackball panel: shared menu checks, axis roles, short/long click and device loss passed");
}
