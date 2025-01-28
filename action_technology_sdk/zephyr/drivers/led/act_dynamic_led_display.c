/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * References:
 *
 * https://www.microbit.co.uk/device/screen
 * https://lancaster-university.github.io/microbit-docs/ubit/display/
 */

#include <zephyr.h>
#include <init.h>
#include <board.h>
#include <drivers/gpio.h>
#include <device.h>
#include <string.h>
#include <soc.h>
#include <errno.h>

#define LOG_LEVEL 2

#include <logging/log.h>

LOG_MODULE_REGISTER(led);

#include <display/led_display.h>

#ifdef CONFIG_CFG_DRV
#include <config.h>
#include <drivers/cfg_drv/driver_config.h>
#endif

struct pwm_chans{
	u8_t chan;
	u8_t gpio_num;
};
struct pwm_chans chan_gpio_num[] = {
							{0, 3}, {0, 4}, {0, 14}, {0, 36}, {0, 49},\
							{1, 5}, {1, 15}, {1, 37}, {1, 50},\
							{2, 6}, {2, 21}, {2, 38}, {2, 51},\
							{3, 7}, {3, 17}, {3, 39}, {3, 52},\
							{4, 8}, {4, 18}, {4, 40}, {4, 53},\
							{5, 9}, {5, 19}, {5, 41}, {5, 54},\
							{6, 10}, {6, 20}, {6, 42}, {6, 55},\
							{7, 11}, {7, 21}, {7, 43}, {7, 45}, {7, 56},\
							{8, 12}, {8, 22}, {8, 44}, {8, 46}, {8, 57},\
};

static u16_t get_chan(int led_index)
{
	for(int i = 0; i < sizeof(chan_gpio_num)/sizeof(struct pwm_chans); i++)
		if(led_index == chan_gpio_num[i].gpio_num)
			return chan_gpio_num[i].chan;

	return -EINVAL;
}

int led_draw_pixel(int led_id, u32_t color, pwm_breath_ctrl_t *ctrl)
{
	int i = 0;
	led_pixel_value pixel;
	int chan;
	int ret;
#ifdef CONFIG_CFG_DRV
	CFG_Type_LED_Drive	led_maps[CFG_MAX_LEDS];
	CFG_Type_LED_Drive *dot = NULL;
	struct device *op_dev;

	memcpy(&pixel, &color, 4);
	ret = cfg_get_by_key(ITEM_LED_LED,&led_maps, sizeof(led_maps));

	if(ret == 0) {
		LOG_ERR("cannot found led map\n");
		return -ENODEV;
	}

	for (i = 0; i < CFG_MAX_LEDS; i ++)	{
		dot = &led_maps[i];
		if (dot->LED_No == (1 << led_id)) {
			break;
		}
	}

	if (!dot) {
		return 0;
	}

	if (dot->GPIO_Pin == 0) {   //spi0_ss pin, can not used in led.
		return 0;
	}

	//printk("dot->LED_No:%d ,led_id:%d, gpio_pin:%d\n",dot->LED_No, led_id, dot->GPIO_Pin);
	switch (pixel.op_code) {
	case LED_OP_OFF:
		if(dot->GPIO_Pin/32 == 0)
			op_dev = device_get_binding("GPIOA");
		else if(dot->GPIO_Pin/32 == 1)
			op_dev = device_get_binding("GPIOB");
		else if(dot->GPIO_Pin/32 == 2)
			op_dev = device_get_binding("GPIOC");

		gpio_pin_configure(op_dev, dot->GPIO_Pin%32, GPIO_OUTPUT_ACTIVE);
		gpio_pin_set(op_dev, dot->GPIO_Pin, !(dot->Active_Level));
		break;
	case LED_OP_ON:
		if(dot->GPIO_Pin/32 == 0)
			op_dev = device_get_binding("GPIOA");
		else if(dot->GPIO_Pin/32 == 1)
			op_dev = device_get_binding("GPIOB");
		else if(dot->GPIO_Pin/32 == 2)
			op_dev = device_get_binding("GPIOC");

		gpio_pin_configure(op_dev, dot->GPIO_Pin%32, GPIO_OUTPUT_ACTIVE);
		gpio_pin_set(op_dev, dot->GPIO_Pin, dot->Active_Level);
		break;
	case LED_OP_BREATH:
#ifdef CONFIG_PWM
		op_dev = device_get_binding("PWM");

		chan = get_chan(dot->GPIO_Pin);
		pwm_pin_mfp_set(op_dev, dot->GPIO_Pin);
		pwm_set_breath_mode(op_dev, chan, ctrl);
#endif
		break;
	case LED_OP_BLINK:
#ifdef CONFIG_PWM
		op_dev = device_get_binding("PWM");

		chan = get_chan(dot->GPIO_Pin);
		pwm_pin_mfp_set(op_dev, dot->GPIO_Pin);
		pwm_pin_set_cycles(op_dev, chan,8*pixel.period, 8*pixel.pulse, pixel.start_state);
		//pwm_pin_set_usec(dot->op_dev, dot->led_pwm, dot->pixel_value.period * 1000, dot->pixel_value.pulse * 1000, pixel.start_state);
#endif
	}
#endif
	return 0;
}

