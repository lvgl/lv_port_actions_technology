/***********************
Head Block of The File
***********************/
#include <string.h>
#include <stdlib.h>
#include <soc.h>
#include <board.h>
#include "gpio.h"

static struct gpio_callback s_gpio_cb = {0};

int Hal_Gpio_Output(uint32_t gpio_num, uint32_t gpio_level)
{
    const struct device *gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(gpio_num));
    if (!gpio_dev) {
        return -EINVAL;
	}

    gpio_pin_configure(gpio_dev, gpio_num % 32, GPIO_OUTPUT);
    gpio_pin_set(gpio_dev, gpio_num % 32, gpio_level);
	// printk("gpio_num %d ,gpio_level %d 0x%x\n",gpio_num,gpio_level,sys_read32(0x40068200)); // SAE_MODI_MAXLIAO_20240516
    return 0;
}

int Hal_Gpio_Level_Get(uint32_t gpio_num)
{
    const struct device *gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(gpio_num));
    if (!gpio_dev) {
        return -1;
	}
    return gpio_pin_get_raw(gpio_dev, gpio_num % 32);
}


int Hal_Gpio_IntInit(uint32_t gpio_num, uint32_t trig_flag, gpio_callback_handler_t callback)
{
	const struct device *gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(gpio_num));
    if (!gpio_dev) {
		return -EINVAL;
	}
 	gpio_pin_configure(gpio_dev, gpio_num % 32, GPIO_INPUT);
    gpio_init_callback(&s_gpio_cb, callback, BIT(gpio_num % 32));
    gpio_add_callback(gpio_dev, &s_gpio_cb);
    gpio_pin_interrupt_configure(gpio_dev, gpio_num % 32, trig_flag);

	return 0;
}

int Hal_Gpio_IntDeInit(uint32_t gpio_num)
{
	const struct device *gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(gpio_num));
    if (!gpio_dev) {
        return -EINVAL;
	}

	gpio_pin_configure(gpio_dev, gpio_num%32, GPIO_OUTPUT | GPIO_PULL_UP);

	return 0;
}
