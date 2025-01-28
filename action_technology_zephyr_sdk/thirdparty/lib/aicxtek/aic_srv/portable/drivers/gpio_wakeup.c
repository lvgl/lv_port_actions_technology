#include <kernel.h>
#include "aic_portable.h"
#include "aic_gpio.h"
#include "aic_gpio_wakeup.h"
#include <sys_wakelock.h>
#include <soc.h>

//#define AIC_GPIO_WAKE_DEBUG
#ifdef AIC_GPIO_WAKE_DEBUG
#define dbg_print alog_info
#else
#define dbg_print(...)
#endif

static volatile int gpio_wake_lock = 0;
static uint32_t request_get_cnt = 0, request_put_cnt = 0;
static uint32_t ack_get_cnt = 0, ack_put_cnt = 0;

void aic_gpio_mcu_unlock(void)
{
    unsigned int key;
    key = irq_lock();
    if(gpio_wake_lock){
        gpio_wake_lock = 0;
        sys_wake_unlock(PARTIAL_WAKE_LOCK);
    }
    irq_unlock(key);
}

void aic_gpio_mcu_lock(void)
{
    unsigned int key;
    key = irq_lock();
    if(!gpio_wake_lock){
        gpio_wake_lock = 1;
        sys_wake_lock(PARTIAL_WAKE_LOCK);
    }
    irq_unlock(key);
}

void aic_gpio_get_request_wakelock(void)
{
    request_get_cnt++;

    aic_gpio_mcu_lock();
}

void aic_gpio_put_request_wakelock(void)
{
    request_put_cnt++;

    if (request_put_cnt != request_get_cnt) {
        alog_error("aic gpio request wakelock get cnt(%d) != put cnt(%d)\n",
                        request_get_cnt, request_put_cnt);
        AIC_ASSERT(0);
    }

    aic_gpio_mcu_unlock();
}

void aic_gpio_get_ack_wakelock(void)
{
    ack_get_cnt++;

    aic_gpio_mcu_lock();
}

void aic_gpio_put_ack_wakelock(void)
{
    ack_put_cnt++;

    if (ack_put_cnt != ack_get_cnt) {
        alog_error("aic gpio ack wakelock get cnt(%d) != put cnt(%d)\n",
                        ack_get_cnt, ack_put_cnt);
        AIC_ASSERT(0);
    }

    aic_gpio_mcu_unlock();
}

void aic_cmux_callin_get_wakelock(void)
{
}

void aic_cmux_callin_put_wakelock(void)
{
}

int aic_gpio_wait_modem_wakeup(u32 timeout)
{
    u32 start_tick, current_tick;

    start_tick = soc_sys_uptime_get();
    while (!aic_gpio_get_modem_wakeup_status()) {
        current_tick = soc_sys_uptime_get();
        if ((current_tick - start_tick) >= timeout) {
            dbg_print("gpio wait modem wakeup timeout!\n");
            return -1;
        }
    }

    return 0;
}
