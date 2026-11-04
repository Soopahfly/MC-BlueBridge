#include "trackball.h"
#include "trackball_motion.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/regs/i2c.h"
#include <string.h>

/* PIM447 register protocol, referenced from Pimoroni's official libraries:
 * https://github.com/pimoroni/trackball-python
 * https://github.com/pimoroni/pimoroni-pico/tree/main/drivers/trackball
 * Original non-blocking C driver; no upstream library code is incorporated.
 * Pointer writes and reads are separate STOP transactions with a 20 ms pause,
 * matching the official Python driver's device timing. */
enum { PROBE_POINTER,PROBE_WAIT,PROBE_READ,INPUT_POINTER,INPUT_WAIT,INPUT_READ,LED_WRITE };
static unsigned job;
static uint8_t tx[5],rx[5],wanted[4],applied[4];
static unsigned tx_length,rx_length,queued,received;
static bool busy,ready,failed,led_valid;
static uint32_t progress,wait_at,next_input,boot_at;
static bool clock_started;
static bb_trackball_motion_t motion;
static bb_trackball_event_t pending;
static i2c_inst_t* bus(void){return BB_TRACKBALL_I2C?i2c1:i2c0;}

static void fail(void){
    i2c_get_hw(bus())->enable=0;
    busy=false;ready=false;failed=true;
    memset(&pending,0,sizeof(pending));
}
static void start(unsigned writes,unsigned reads,uint32_t now){
    i2c_hw_t* hw=i2c_get_hw(bus());
    (void)hw->clr_stop_det;(void)hw->clr_tx_abrt;
    tx_length=writes;rx_length=reads;queued=received=0;progress=now;busy=true;
}
static void complete(uint32_t now){
    switch(job){
        case PROBE_POINTER:job=PROBE_WAIT;wait_at=now;break;
        case PROBE_READ:
            if(rx[0]!=0x11||rx[1]!=0xba){fail();return;}
            ready=true;job=INPUT_POINTER;next_input=now;break;
        case INPUT_POINTER:job=INPUT_WAIT;wait_at=now;break;
        case INPUT_READ:{
            bb_trackball_event_t e;
            bb_trackball_decode(&motion,rx,now,BB_TRACKBALL_COUNTS_PER_STEP,
                BB_TRACKBALL_INVERT_X,BB_TRACKBALL_INVERT_Y,&e);
            pending.x+=e.x;pending.y+=e.y;pending.down=e.down;
            if(e.clicks)pending.clicks=1;
            job=LED_WRITE;next_input=now+10u;break;
        }
        case LED_WRITE:
            /* Commit bytes actually sent, not a colour changed during transfer. */
            memcpy(applied,tx+1,4);led_valid=true;job=INPUT_POINTER;break;
    }
}
void bb_trackball_init(void){
    memset(&motion,0,sizeof(motion));memset(&pending,0,sizeof(pending));
    memset(wanted,0,sizeof(wanted));
    busy=ready=failed=led_valid=clock_started=false;
    job=PROBE_POINTER;
    i2c_init(bus(),100000);
    gpio_set_function(BB_TRACKBALL_SDA,GPIO_FUNC_I2C);
    gpio_set_function(BB_TRACKBALL_SCL,GPIO_FUNC_I2C);
    gpio_pull_up(BB_TRACKBALL_SDA);gpio_pull_up(BB_TRACKBALL_SCL);
    i2c_hw_t* hw=i2c_get_hw(bus());
    hw->enable=0;hw->tar=BB_TRACKBALL_ADDRESS;hw->enable=1;
}
bool bb_trackball_available(void){return ready;}
void bb_trackball_colour(uint8_t r,uint8_t g,uint8_t b,uint8_t w){wanted[0]=r;wanted[1]=g;wanted[2]=b;wanted[3]=w;}
void bb_trackball_take(bb_trackball_event_t* e){*e=pending;pending.x=pending.y=0;pending.clicks=0;}

void bb_trackball_poll(uint32_t now){
    if(failed)return;
    if(!clock_started){boot_at=now;clock_started=true;}
    if((uint32_t)(now-boot_at)<100u)return;
    i2c_hw_t* hw=i2c_get_hw(bus());
    if(busy){
        if(hw->raw_intr_stat&I2C_IC_RAW_INTR_STAT_TX_ABRT_BITS){fail();return;}
        unsigned old_queued=queued,old_received=received;
        /* All requests fit in the 16-entry hardware FIFOs. Bound both loops. */
        unsigned total=tx_length?tx_length:rx_length;
        for(unsigned n=0;n<5&&queued<total;n++){
            if(!(hw->status&I2C_IC_STATUS_TFNF_BITS))break;
            uint32_t command=tx_length?tx[queued]:I2C_IC_DATA_CMD_CMD_BITS;
            if(++queued==total)command|=I2C_IC_DATA_CMD_STOP_BITS;
            hw->data_cmd=command;
        }
        for(unsigned n=0;n<5&&received<rx_length;n++){
            if(!(hw->status&I2C_IC_STATUS_RFNE_BITS))break;
            rx[received++]=(uint8_t)hw->data_cmd;
        }
        if(old_queued!=queued||old_received!=received)progress=now;
        if(queued!=total||received!=rx_length||!(hw->raw_intr_stat&I2C_IC_RAW_INTR_STAT_STOP_DET_BITS)){
            if((uint32_t)(now-progress)>=100u)fail();
            return;
        }
        (void)hw->clr_stop_det;busy=false;complete(now);
        return;
    }
    switch(job){
        case PROBE_POINTER:tx[0]=0xfa;start(1,0,now);break;
        case PROBE_WAIT:if((uint32_t)(now-wait_at)>=20u){job=PROBE_READ;start(0,2,now);}break;
        case INPUT_POINTER:
            if((int32_t)(now-next_input)>=0){tx[0]=0x04;start(1,0,now);}
            break;
        case INPUT_WAIT:if((uint32_t)(now-wait_at)>=20u){job=INPUT_READ;start(0,5,now);}break;
        case LED_WRITE:
            if(!led_valid||memcmp(wanted,applied,4)){tx[0]=0;memcpy(tx+1,wanted,4);start(5,0,now);}
            else job=INPUT_POINTER;
            break;
    }
}
