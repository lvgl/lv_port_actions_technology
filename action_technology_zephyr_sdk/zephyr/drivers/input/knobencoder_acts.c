/*
 * Copyright (c) 2022 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief knob encoder driver for Actions SoC
 */
#include <errno.h>
#include <kernel.h>
#include <string.h>
#include <irq.h>
#include <drivers/input/input_dev.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <board.h>
#include <logging/log.h>

static const struct acts_pin_config pins_knobencoder[] = {CONFIG_KNOBGPIO_INIA,CONFIG_KNOBGPIO_INIB};

enum KNOB_ACTION_TYPE
{
    KONB_NO_ACTION = 0,
    KONB_CLOCKWISE_PRE,
    KONB_CLOCKWISE, 
    KONB_ANTICLOCKWISE_PRE, 
    KONB_ANTICLOCKWISE,
};

struct acts_knobencoder_data {
	input_notify_t notify;
	const struct device *this_dev;
    struct gpio_callback key_gpio_cb[ARRAY_SIZE(pins_knobencoder)];
    uint8_t action;
    uint8_t rotation_speed;
};

struct acts_knobencoder_config {
	uint8_t pinmux_size;
	const struct acts_pin_config *pinmux;
};


static struct acts_knobencoder_data knobencoder_acts_data;


static const struct acts_knobencoder_config knobencoder_acts_config = {
    .pinmux_size = ARRAY_SIZE(pins_knobencoder),
	.pinmux = pins_knobencoder,
};



static void knob_acts_register_notify(const struct device *dev, input_notify_t notify)
{
	struct acts_knobencoder_data *knobkey = dev->data;

	// LOG_DBG("register notify 0x%x", (uint32_t)notify);

	knobkey->notify = notify;
}

static void knob_acts_unregister_notify(const struct device *dev, input_notify_t notify)
{
	struct acts_knobencoder_data *knobkey = dev->data;

	// LOG_DBG("unregister notify 0x%x", (uint32_t)notify);

	knobkey->notify = NULL;
}

static void knob_acts_enable(const struct device *dev)
{
    const struct acts_knobencoder_config *cfg = dev->config;
    const struct acts_pin_config *pinconf = cfg->pinmux;
    uint8_t i;

    for(i = 0; i < cfg->pinmux_size; i++)
    {
        const struct device *gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(pinconf[i].pin_num));
        if (!gpio_dev) {
            printk("failed to bind GPIO:%d\n", pinconf[i].pin_num);
        }
        gpio_pin_interrupt_configure(gpio_dev, (pinconf[i].pin_num % 32), GPIO_INT_EDGE_FALLING);
    }
}

static void knob_acts_disable(const struct device *dev)
{
    const struct acts_knobencoder_config *cfg = dev->config;
    const struct acts_pin_config *pinconf = cfg->pinmux;
    uint8_t i;

    for(i = 0; i < cfg->pinmux_size; i++)
    {
        const struct device *gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(pinconf[i].pin_num));
        if (!gpio_dev) {
            printk("failed to bind GPIO:%d\n", pinconf[i].pin_num);
        }
        gpio_pin_interrupt_configure(gpio_dev, (pinconf[i].pin_num % 32), GPIO_INT_DISABLE);
    }
}

static void KNOB_IRQ_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
    const struct device *gpio_devA = device_get_binding(CONFIG_GPIO_PIN2NAME(pins_knobencoder[0].pin_num));
    const struct device *gpio_devB = device_get_binding(CONFIG_GPIO_PIN2NAME(pins_knobencoder[1].pin_num));

    if(cb->pin_mask == (1<<(pins_knobencoder[0].pin_num % 32)))
    {
        if(!gpio_pin_get(gpio_devA, pins_knobencoder[0].pin_num % 32))
        {
            gpio_pin_interrupt_configure(gpio_devA, (pins_knobencoder[0].pin_num % 32), GPIO_INT_EDGE_RISING);
            if(!gpio_pin_get(gpio_devB, pins_knobencoder[1].pin_num % 32))
            {
                //gpio_pin_interrupt_configure(gpio_devB, (pins_knobencoder[1].pin_num % 32), GPIO_INT_DISABLE);
                knobencoder_acts_data.action = KONB_CLOCKWISE_PRE;
            }
        }
        else
        {
            gpio_pin_interrupt_configure(gpio_devA, (pins_knobencoder[0].pin_num % 32), GPIO_INT_EDGE_FALLING);
            if((knobencoder_acts_data.action == KONB_CLOCKWISE_PRE) &&
            gpio_pin_get(gpio_devB, pins_knobencoder[1].pin_num % 32))
            {
                // printk("KONB_CLOCKWISE\n");
                knobencoder_acts_data.action = KONB_CLOCKWISE;

                if(knobencoder_acts_data.notify) {
                    struct input_value val_notify = {0};
                    val_notify.keypad.value = KEY_KONB_CLOCKWISE;
                    knobencoder_acts_data.notify(NULL, &val_notify);
                }

                knobencoder_acts_data.action = KONB_NO_ACTION;
                gpio_pin_interrupt_configure(gpio_devB, (pins_knobencoder[1].pin_num % 32), GPIO_INT_EDGE_FALLING);
            }
        }
    }
    else if (cb->pin_mask == (1<<(pins_knobencoder[1].pin_num % 32)))
    {
        if(!gpio_pin_get(gpio_devB, pins_knobencoder[1].pin_num % 32))
        {
            gpio_pin_interrupt_configure(gpio_devB, (pins_knobencoder[1].pin_num % 32), GPIO_INT_EDGE_RISING);
            if(!gpio_pin_get(gpio_devA, pins_knobencoder[0].pin_num % 32))
            {
                //gpio_pin_interrupt_configure(gpio_devA, (pins_knobencoder[0].pin_num % 32), GPIO_INT_DISABLE);
                knobencoder_acts_data.action = KONB_ANTICLOCKWISE_PRE;
            }
        }
        else
        {
            gpio_pin_interrupt_configure(gpio_devB, (pins_knobencoder[1].pin_num % 32), GPIO_INT_EDGE_FALLING);
            if((knobencoder_acts_data.action == KONB_ANTICLOCKWISE_PRE) &&
            gpio_pin_get(gpio_devA, pins_knobencoder[0].pin_num % 32))
            {
                // printk("KONB_ANTICLOCKWISE\n");
                knobencoder_acts_data.action = KONB_ANTICLOCKWISE;
                if(knobencoder_acts_data.notify) {
                    struct input_value val_notify = {0};
                    val_notify.keypad.value = KEY_KONB_ANTICLOCKWISE;
                    knobencoder_acts_data.notify(NULL, &val_notify);
                }

                knobencoder_acts_data.action = KONB_NO_ACTION;
                gpio_pin_interrupt_configure(gpio_devA, (pins_knobencoder[0].pin_num % 32), GPIO_INT_EDGE_FALLING);
            }
            
        }
    }
  
}

const struct input_dev_driver_api knobencoder_acts_driver_api = {
	.enable = knob_acts_enable,
	.disable = knob_acts_disable,
	.register_notify = knob_acts_register_notify,
	.unregister_notify = knob_acts_unregister_notify,
};

int knobencoder_acts_init(const struct device *dev)
{
    const struct acts_knobencoder_config *cfg = dev->config;
    const struct acts_pin_config *pinconf = cfg->pinmux;
    struct acts_knobencoder_data *knobencoder = dev->data;
    uint8_t i;

    for(i = 0; i < cfg->pinmux_size; i++)
    {
        const struct device *gpio_dev = device_get_binding(CONFIG_GPIO_PIN2NAME(pinconf[i].pin_num));
        if (!gpio_dev) {
            printk("failed to bind GPIO:%d\n", pinconf[i].pin_num);
        }
        gpio_pin_configure(gpio_dev, (pinconf[i].pin_num % 32), (GPIO_PULL_UP | GPIO_INPUT | GPIO_INT_DEBOUNCE));
        gpio_pin_interrupt_configure(gpio_dev, (pinconf[i].pin_num % 32), GPIO_INT_EDGE_FALLING);
        gpio_init_callback(&knobencoder->key_gpio_cb[i], KNOB_IRQ_callback, BIT(pinconf[i].pin_num % 32));
        gpio_add_callback(gpio_dev, &knobencoder->key_gpio_cb[i]);
    }
	return 0;
}

#if IS_ENABLED(CONFIG_KNOB_ENCODER)
DEVICE_DEFINE(knobencoder, CONFIG_KNOB_ENCODER_DEV_NAME,
		    knobencoder_acts_init, NULL,
		    &knobencoder_acts_data, &knobencoder_acts_config,
		    POST_KERNEL, 60,
		    &knobencoder_acts_driver_api);
#endif

