#include <aic_type.h>
#include <aic_portable.h>
#include <aic_gpio_wakeup.h>
#include <aic_cfg.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <board.h>

//#define AIC_GPIO_DEBUG
#ifdef AIC_GPIO_DEBUG
#define dbg_print   printk
#else
#define dbg_print(...)
#endif

static struct gpio_callback s_aic_gpio_cb = {0};
static int s_gpio_level = 1;

/*
    gpio0-gpio95    : 0-95
    wio0-4          : 96-100
*/
static const struct device * aic_gpio_get_dev(u32 gpio_num)
{
    const struct device *gpio_dev = NULL;
    if(gpio_num < 32)
        gpio_dev = device_get_binding(CONFIG_GPIO_A_NAME);
    else if(gpio_num < 64)
        gpio_dev = device_get_binding(CONFIG_GPIO_B_NAME);
    else if(gpio_num < 96)
        gpio_dev = device_get_binding(CONFIG_GPIO_C_NAME);
    else{
        if(gpio_num < 101)
            gpio_dev = device_get_binding(CONFIG_WIO_NAME);
    }
    return gpio_dev;
}

static void aic_gpio_callback(const struct device *port,
                                struct gpio_callback *cb, uint32_t pins)
{
    aic_gpio_wakeup_cfg_t *gpio = aic_gpio_wakeup_cfg();
    const struct device *dev;
    u32 gpio_num;
    int pin_state;

    gpio_num = gpio->modem_wakeup_mcu.gpio_num;
    dev = aic_gpio_get_dev(gpio_num);

    pin_state = gpio_pin_get_raw(dev, gpio_num%32);
    dbg_print("aic_gpio_callback %d:%d\n", gpio_num, pin_state);

    if (s_gpio_level != pin_state) {
        alog_error("gpio_level %d != pin state %d\n", s_gpio_level, pin_state);
        return;
    }

    s_gpio_level = (pin_state == 0 ? 1 : 0);

    if (pin_state == 0)
        aic_gpio_mcu_can_sleep();
    else if (pin_state == 1)
        aic_gpio_mcu_keep_wakeup();
    else
        AIC_ASSERT(0);
}

int aic_gpio_set_dir(u32 gpio_num, u32 gpio_dir)
{
    const struct device *dev = aic_gpio_get_dev(gpio_num);
    if(dev == NULL)
        return -1;

    dbg_print("aic_gpio_set_dir:%d, %d\n", gpio_num, gpio_dir);

    if(gpio_dir) {
        gpio_pin_configure(dev, gpio_num%32, GPIO_INPUT);
        gpio_init_callback(&s_aic_gpio_cb, aic_gpio_callback, BIT(gpio_num%32));
        gpio_add_callback(dev, &s_aic_gpio_cb);
        gpio_pin_interrupt_configure(dev, gpio_num%32, GPIO_INT_EDGE_BOTH);
    } else {
        gpio_pin_configure(dev, gpio_num%32, GPIO_OUTPUT|GPIO_PULL_UP);
    }

    return 0;
}

int aic_gpio_set_outvalue(u32 gpio_num, u32 gpio_value)
{
    const struct device *dev = aic_gpio_get_dev(gpio_num);
    if(dev == NULL)
        return -1;

    dbg_print("aic_gpio_set_outvalue:%d, %d\n", gpio_num, gpio_value);

    gpio_pin_set_raw(dev, gpio_num%32, gpio_value);

    return 0;
}

int aic_gpio_level_get(u32 gpio_id)
{
    const struct device *dev = aic_gpio_get_dev(gpio_id);

    if(dev == NULL)
        return -1;

    return gpio_pin_get_raw(dev, gpio_id%32);
}

void aic_gpio_init(void)
{
    aic_gpio_wakeup_cfg_t *gpio = aic_gpio_wakeup_cfg();
    const struct device *dev;
    u32 gpio_num;

    /* config modem wakeup mcu gpio */
    aic_gpio_set_dir(gpio->modem_wakeup_mcu.gpio_num, 1);

    /* config power on modem gpio */
    gpio_num = 60;
    dev = aic_gpio_get_dev(gpio_num);
    gpio_pin_configure(dev, gpio_num%32, GPIO_OUTPUT_HIGH);

    printk("aic gpio init sucess!\n");
}

#if 0
void aic_gpio_test_entry(void *arg1, void *arg2, void *arg3)
{
    aic_gpio_wakeup_cfg_t *gpio = aic_gpio_wakeup_cfg();

    aic_gpio_set_dir(gpio->modem_wakeup_mcu.gpio_num, 1);
    aic_gpio_set_dir(gpio->mcu_wakeup_modem.gpio_num, 0);
    aic_gpio_set_outvalue(gpio->mcu_wakeup_modem.gpio_num, 0);

    while (1) {
        k_msleep(300);
        aic_gpio_set_outvalue(gpio->mcu_wakeup_modem.gpio_num, 1);
        k_msleep(300);
        aic_gpio_set_outvalue(gpio->mcu_wakeup_modem.gpio_num, 0);
    }
}

K_THREAD_STACK_DEFINE(gpio_test_stack, 1024);
struct k_thread gpio_test_data;
static int aic_gpio_test_init(const struct device *arg)
{
    k_tid_t gpio_test_id;
    gpio_test_id = k_thread_create(&gpio_test_data, gpio_test_stack,
			      K_THREAD_STACK_SIZEOF(gpio_test_stack),
			      aic_gpio_test_entry, NULL, NULL, NULL,
			      10, 0, K_NO_WAIT);

    alog_warn("aic_gpio_test_init() success\n");

    return 0;
}
SYS_INIT(aic_gpio_test_init, APPLICATION, 50);
#endif
