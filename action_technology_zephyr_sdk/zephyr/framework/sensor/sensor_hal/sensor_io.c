/*******************************************************************************
 * @file    sensor_io.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-08-12
 * @brief   sensor testing module
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <sensor_hal.h>
#include <sensor_io.h>
#include <soc.h>
#include <zephyr.h>
#include <drivers/gpio.h>
#include <os_common_api.h>

/******************************************************************************/
//constants
/******************************************************************************/
// CFG_IRQ
#define CFG_IRQ_ON	(GPIO_INT_ENABLE | GPIO_INT_EDGE | GPIO_INT_HIGH_1)
#define CFG_IRQ_OFF	(GPIO_INT_DISABLE)

// TIMER
#define CMU_TIMERCLK(id)			(CMU_TIMER0CLK + (id) * 4)
#define T_CTL(id)					(T0_CTL + (id) * 0x20)
#define T_VAL(id)					(T0_VAL + (id) * 0x20)
#define T_CNT(id)					(T0_CNT + (id) * 0x20)

typedef struct sensor_tm_s {
	uint32_t tid;	// timer id
	uint32_t sid;	// sensor id
} sensor_tm_t;

/******************************************************************************/
//variables
/******************************************************************************/
static struct device *gpio_dev[4] =  { NULL };
static sensor_io_cb_t sensor_io_cb[NUM_SENSOR] = { NULL };
static struct gpio_callback sensor_gpio_cb[NUM_SENSOR] = { NULL };
#if defined(CONFIG_SOC_SERIES_LEOPARD)
static sensor_tm_t sensor_tm[6] =  { 0 };
#else
static sensor_tm_t sensor_tm[5] =  { 0 };
#endif

/******************************************************************************/
//functions
/******************************************************************************/
void sensor_io_init(const sensor_dev_t *dev)
{
    int pin, grp;

    // init int pin
    if (dev->io.int_io > 0) {
        grp = (dev->io.int_io / 32);
        pin = (dev->io.int_io % 32);
        gpio_pin_interrupt_configure(gpio_dev[grp], pin,	CFG_IRQ_OFF);
    }

    // power on
    if (dev->io.pwr_io > 0) {
        grp = (dev->io.pwr_io / 32);
        pin = (dev->io.pwr_io % 32);
        gpio_pin_configure(gpio_dev[grp], pin,	GPIO_OUTPUT);
        gpio_pin_set(gpio_dev[grp], pin, dev->io.pwr_val);
    }

    // reset
    if (dev->io.rst_io > 0) {
        grp = (dev->io.rst_io / 32);
        pin = (dev->io.rst_io % 32);
        gpio_pin_configure(gpio_dev[grp], pin,	GPIO_OUTPUT);
        gpio_pin_set(gpio_dev[grp], pin, dev->io.rst_val);
        k_msleep(dev->io.rst_lt);
        gpio_pin_set(gpio_dev[grp], pin, !dev->io.rst_val);
        k_msleep(dev->io.rst_ht);
    }
}

void sensor_io_deinit(const sensor_dev_t *dev)
{
    int pin, grp;

    // reset on
    if (dev->io.rst_io > 0) {
        grp = (dev->io.rst_io / 32);
        pin = (dev->io.rst_io % 32);
        gpio_pin_set(gpio_dev[grp], pin, dev->io.rst_val);
    }

    // power down
    if (dev->io.pwr_io > 0) {
        grp = (dev->io.pwr_io / 32);
        pin = (dev->io.pwr_io % 32);
        gpio_pin_set(gpio_dev[grp], pin, !dev->io.pwr_val);
    }
}

static void sensor_gpio_callback(const struct device *port,
                    struct gpio_callback *cb, gpio_port_pins_t pins)
{
    int idx;

    for (idx = 0; idx < NUM_SENSOR; idx ++) {
        if (cb == &sensor_gpio_cb[idx]) {
            sensor_io_cb[idx](pins, idx);
        }
    }
}

static void senosr_tm_callback(void *arg)
{
    sensor_tm_t *tm = &sensor_tm[(uint32_t)arg];
    
    if ((uint32_t)arg >= 6) {
        return;
    }

    sensor_io_clear_tm_pending(tm->tid);

    // callback
    sensor_io_cb[tm->sid](tm->sid, tm->sid);
}

static void sensor_set_irq_mode(uint8_t int_io, uint8_t int_mode)
{
	uint32_t val;
	
    val = sys_read32(GPION_CTL(int_io));
	val = (val & ~GPIO_CTL_INC_TRIGGER_MASK) | GPIO_CTL_INC_TRIGGER(int_mode);
	sys_write32(val, GPION_CTL(int_io));
}

static void sensor_tm_config(uint32_t tid, uint32_t ms, int irq)
{
    uint32_t val;

    if (ms > 0) {
        val = soc_rc32K_freq() * ms / 1000;
    } else {
        val = TIMER_MAX_CYCLES_VALUE;
    }

    sys_write32(0x4, CMU_TIMERCLK(tid)); // select RC32K
    acts_clock_peripheral_enable(CLOCK_ID_TIMER0 + tid);
    acts_reset_peripheral(CLOCK_ID_TIMER0 + tid);

    sys_write32(0x1, T_CTL(tid)); // stop and clear pending
    timer_reg_wait();
    sys_write32(val, T_VAL(tid)); // set value

    if (irq) {
        sys_write32(0x822, T_CTL(tid)); // counter up, irq enable
    } else {
        sys_write32(0x824, T_CTL(tid)); // counter up,  reload, irq disable
    }
}

#if defined(CONFIG_SOC_SERIES_LEOPARD)
static void sensor_tm_enable_irq(uint32_t tid, uint32_t ms, int id, int irq)
{
    if (tid == TIMER5) {
        tid = 5;
    }
    sensor_tm_config(tid, ms, irq);
    sensor_tm[tid].tid = tid;
    sensor_tm[tid].sid = id;
    if (irq) {
        if (tid == 5) {
            irq_enable(IRQ_ID_TIMER5);
        } else {
            irq_enable(IRQ_ID_TIMER0 + tid);
        }
    }
}

static void sensor_tm_disable_irq(uint32_t tid)
{
    if (tid == TIMER5) {
        tid = 5;
    }
    sys_write32(0x1, T_CTL(tid)); // stop timer and clear pending
    if (tid == 5) {
        irq_disable(IRQ_ID_TIMER5);
    } else {
        irq_disable(IRQ_ID_TIMER0 + tid);
    }
}
#else
static void sensor_tm_enable_irq(uint32_t tid, uint32_t ms, int id, int irq)
{
    sensor_tm_config(tid, ms, irq);
    sensor_tm[tid].tid = tid;
    sensor_tm[tid].sid = id;
    if (irq) {
        irq_enable(IRQ_ID_TIMER0 + tid);
    }
}

static void sensor_tm_disable_irq(uint32_t tid)
{
    sys_write32(0x1, T_CTL(tid)); // stop timer and clear pending
    irq_disable(IRQ_ID_TIMER0 + tid);

}
#endif
void sensor_io_enable_irq(const sensor_dev_t *dev, sensor_io_cb_t cb, int id)
{
    int pin, grp;
    task_trig_t *task_trig = (task_trig_t*)dev->task;

    sensor_io_cb[id] = cb;

    if (dev->io.int_io > 0) {
        // enable gpio irq
        grp = (dev->io.int_io / 32);
        pin = (dev->io.int_io % 32);
        acts_pinmux_set(dev->io.int_io, 0);
        gpio_pin_configure(gpio_dev[grp], pin, GPIO_INPUT);
        gpio_pin_interrupt_configure(gpio_dev[grp], pin, CFG_IRQ_ON);
        gpio_init_callback(&sensor_gpio_cb[id], sensor_gpio_callback, (1 << pin));
        gpio_add_callback(gpio_dev[grp], &sensor_gpio_cb[id]);
        sensor_set_irq_mode(dev->io.int_io, dev->io.int_mode);
    } else {
        // enable timer irq
#if defined(CONFIG_SOC_SERIES_LEOPARD)
        if (task_trig && ((task_trig->trig >= TIMER3 && task_trig->trig <= TIMER4) || task_trig->trig == TIMER5)) {
#else
        if (task_trig && task_trig->trig >= TIMER3 && task_trig->trig <= TIMER4) {
#endif
            sensor_tm_enable_irq(task_trig->trig, task_trig->peri, id, 1);
        }

    }
}

void sensor_io_disable_irq(const sensor_dev_t *dev, sensor_io_cb_t cb, int id)
{
    int pin, grp;
    task_trig_t *task_trig = (task_trig_t*)dev->task;

    if (dev->io.int_io > 0) {
        // disable gpio irq
        grp = (dev->io.int_io / 32);
        pin = (dev->io.int_io % 32);
        gpio_pin_interrupt_configure(gpio_dev[grp], pin, CFG_IRQ_OFF);
        gpio_init_callback(&sensor_gpio_cb[id], sensor_gpio_callback, (1 << pin));
        gpio_remove_callback(gpio_dev[grp], &sensor_gpio_cb[id]);
    } else {
        // disable timer irq
#if defined(CONFIG_SOC_SERIES_LEOPARD)
        if (task_trig && ((task_trig->trig >= TIMER3 && task_trig->trig <= TIMER4) || task_trig->trig == TIMER5)) {
#else
        if (task_trig && task_trig->trig >= TIMER3 && task_trig->trig <= TIMER4) {
#endif
            sensor_tm_disable_irq(task_trig->trig);
        }
    }

    sensor_io_cb[id] = NULL;
}

void sensor_io_enable_trig(const sensor_dev_t *dev, int id)
{
    int pin, grp;
    task_trig_t *task_trig = (task_trig_t*)dev->task;

    if (!task_trig || !task_trig->en) {
        return;
    }

    if (dev->io.int_io > 0) {
        // enable gpio trigger
        grp = (dev->io.int_io / 32);
        pin = (dev->io.int_io % 32);
        gpio_pin_configure(gpio_dev[grp], pin, GPIO_INPUT);
        sensor_set_irq_mode(dev->io.int_io, dev->io.int_mode);
        acts_pinmux_set(dev->io.int_io, 11);
    } else {
        // enable timer trigger
#if defined(CONFIG_SOC_SERIES_LEOPARD)
        if (task_trig && ((task_trig->trig >= TIMER3 && task_trig->trig <= TIMER4) || task_trig->trig == TIMER5)) {
#else
        if (task_trig && task_trig->trig >= TIMER3 && task_trig->trig <= TIMER4) {
#endif
            sensor_tm_enable_irq(task_trig->trig, task_trig->peri, id, 0);
        }
    }
}

void sensor_io_disable_trig(const sensor_dev_t *dev, int id)
{
    task_trig_t *task_trig = (task_trig_t*)dev->task;

    if (!task_trig || !task_trig->en) {
        return;
    }

    if (dev->io.int_io > 0) {
        // disable gpio trigger
        acts_pinmux_set(dev->io.int_io, 0);
    } else {
        // disable timer trigger
#if defined(CONFIG_SOC_SERIES_LEOPARD)
        if (task_trig && ((task_trig->trig >= TIMER3 && task_trig->trig <= TIMER4) || task_trig->trig == TIMER5)) {
#else
        if (task_trig && task_trig->trig >= TIMER3 && task_trig->trig <= TIMER4) {
#endif
            sensor_tm_disable_irq(task_trig->trig);
        }
    }
}

void sensor_io_config_tm(const sensor_dev_t *dev, int ms)
{
    task_trig_t *task_trig = (task_trig_t*)dev->task;
    int irq = task_trig && !task_trig->en && (dev->io.int_io == 0);

    // change timer period
#if defined(CONFIG_SOC_SERIES_LEOPARD)
        if (task_trig && ((task_trig->trig >= TIMER3 && task_trig->trig <= TIMER4) || task_trig->trig == TIMER5)) {
#else
        if (task_trig && task_trig->trig >= TIMER3 && task_trig->trig <= TIMER4) {
#endif
        sensor_tm_config(task_trig->trig, ms, irq);
    }
}

uint32_t sensor_io_get_tm(const sensor_dev_t *dev)
{
    task_trig_t *task_trig = (task_trig_t*)dev->task;
    uint32_t cycle = 0;

    // get timer cycle
#if defined(CONFIG_SOC_SERIES_LEOPARD)
    if (task_trig && ((task_trig->trig >= TIMER3 && task_trig->trig <= TIMER4) || task_trig->trig == TIMER5)) {
        if (task_trig->trig == TIMER5)
            task_trig->trig = 5;
#else
    if (task_trig && task_trig->trig >= TIMER3 && task_trig->trig <= TIMER4) {
#endif
        cycle = sys_read32(T_CNT(task_trig->trig));
    }

    return cycle;
}

void sensor_io_clear_tm_pending(int tid)
{
    uint32_t time_val = sys_read32(T_VAL(tid));
    acts_clock_peripheral_disable(CLOCK_ID_TIMER0 + tid); //clk disable 
    sys_write32(0x0, CMU_TIMERCLK(tid)); // select HOSC
    acts_clock_peripheral_enable(CLOCK_ID_TIMER0 + tid);

    // clear pending
    sys_write32(0x1, T_CTL(tid)); // stop and clear pending
    soc_udelay(1);

    acts_clock_peripheral_disable(CLOCK_ID_TIMER0 + tid); //clk disable 
    sys_write32(0x4, CMU_TIMERCLK(tid)); // select RC32K
    acts_clock_peripheral_enable(CLOCK_ID_TIMER0 + tid);
    sys_write32(time_val, T_VAL(tid)); // set value
    sys_write32(0x822, T_CTL(tid)); // counter up,  reload, irq enable
}

static int sensor_io_dev_init(const struct device *dev)
{
    ARG_UNUSED(dev);

    /* get device */
    gpio_dev[0] = (struct device*)device_get_binding("GPIOA");
    if (!gpio_dev[0]) {
        SYS_LOG_ERR("[gpioa] get device failed!");
        return -1;
    }

    /* get device */
    gpio_dev[1] = (struct device*)device_get_binding("GPIOB");
    if (!gpio_dev[1]) {
        SYS_LOG_ERR("[gpiob] get device failed!");
        return -1;
    }

    /* get device */
    gpio_dev[2] = (struct device*)device_get_binding("GPIOC");
    if (!gpio_dev[2]) {
        //SYS_LOG_ERR("[gpioc] get device failed!");
        //return -1;
    }

    /* timer irq callback */
    IRQ_CONNECT(IRQ_ID_TIMER3, 0, senosr_tm_callback, 3, 0);
    IRQ_CONNECT(IRQ_ID_TIMER4, 0, senosr_tm_callback, 4, 0);
#if defined(CONFIG_SOC_SERIES_LEOPARD)
    IRQ_CONNECT(IRQ_ID_TIMER5, 0, senosr_tm_callback, 5, 0);
#endif
    return 0;
}

SYS_INIT(sensor_io_dev_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

