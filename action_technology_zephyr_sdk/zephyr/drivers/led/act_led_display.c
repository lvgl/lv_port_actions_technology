/*
 * Copyright (c) 2017 Intel Corporation
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

#define MAX_DUTY (32 * 255)

#ifdef BOARD_LED_MAP

const led_map ledmaps[] = {
    BOARD_LED_MAP
};
	
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

typedef struct
{
	u8_t led_id;
	u8_t led_pin;
	u8_t led_pwm;
	u8_t active_level;
	u8_t state;
	led_pixel_value    pixel_value;
	struct device *op_dev;
}led_dot;

typedef struct
{
	u16_t led_dot_num;
	led_dot pixel[ARRAY_SIZE(ledmaps)];
}led_panel;

static led_panel led;

static u16_t get_chan(int led_index)
{
	for(int i = 0; i < sizeof(chan_gpio_num)/sizeof(struct pwm_chans); i++)
		if(led_index == chan_gpio_num[i].gpio_num)
			return chan_gpio_num[i].chan;

	return -EINVAL;
}

static int led_display_init(struct device *dev)
{
	int i = 0;
	ARG_UNUSED(dev);

	led.led_dot_num = ARRAY_SIZE(ledmaps);

	for (i = 0; i < led.led_dot_num; i ++) {
		led_dot *dot = &led.pixel[i];
		dot->led_pin =  ledmaps[i].led_pin;
		dot->led_id = ledmaps[i].led_id;
		dot->active_level = ledmaps[i].active_level;
		dot->led_pwm =  ledmaps[i].led_pwm;
		if (dot->led_pwm != 0xff) {
		#ifdef CONFIG_PWM
			dot->op_dev = device_get_binding("PWM");
		#endif
		} else {
			if(dot->led_pin/32 == 0)
				dot->op_dev = device_get_binding("GPIOA");
			else if(dot->led_pin/32 == 1)
				dot->op_dev = device_get_binding("GPIOB");
			else if(dot->led_pin/32 == 2)
				dot->op_dev = device_get_binding("GPIOC");

			gpio_pin_configure(dot->op_dev, dot->led_pin%32, GPIO_OUTPUT_ACTIVE);
		}
		if (!dot->op_dev) {
			LOG_ERR("cannot found dev gpio \n");
			goto err_exit;
		}
		memset(&dot->pixel_value, 0 , sizeof(dot->pixel_value));
	}

	return 0;

err_exit:

	return -1;
}

SYS_INIT(led_display_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif

int led_draw_pixel(int led_id, u32_t color, pwm_breath_ctrl_t *ctrl)
{
#ifdef BOARD_LED_MAP
	int i = 0;
	led_dot *dot = NULL;
	led_pixel_value pixel;
	int chan;

	memcpy(&pixel, &color, 4);

	for (i = 0; i < led.led_dot_num; i ++)	{
		dot = &led.pixel[i];
		if (dot->led_id == led_id) {
			break;
		}
	}

	if (!dot) {
		return 0;
	}
	dot->pixel_value = pixel;

	switch (pixel.op_code) {
	case LED_OP_OFF:
		if (dot->led_pwm) {
		#ifdef CONFIG_PWM
			chan = get_chan(dot->led_pin);
			if (dot->active_level) {
				pwm_pin_set_cycles(dot->op_dev, chan, MAX_DUTY, 0, START_VOLTAGE_HIGH);
			} else {
				pwm_pin_set_cycles(dot->op_dev, chan, MAX_DUTY, MAX_DUTY, START_VOLTAGE_HIGH);
			}
		#endif
		} else {
			gpio_pin_set(dot->op_dev, dot->led_pin, !(dot->active_level));
		}

		break;
	case LED_OP_ON:
		if (dot->led_pwm) {
		#ifdef CONFIG_PWM
			chan = get_chan(dot->led_pin);
			if (dot->active_level) {
				pwm_pin_set_cycles(dot->op_dev, chan, MAX_DUTY, MAX_DUTY, START_VOLTAGE_HIGH);
			} else {
				pwm_pin_set_cycles(dot->op_dev, chan, MAX_DUTY, 0, START_VOLTAGE_HIGH);
			}
		#endif
		} else {
			gpio_pin_set(dot->op_dev, dot->led_pin, dot->active_level);
		}

		break;
	case LED_OP_BREATH:
		if (dot->led_pwm) {
		#ifdef CONFIG_PWM
			chan = get_chan(dot->led_pin);
			pwm_set_breath_mode(dot->op_dev, chan, ctrl);
		#endif
		}

		break;
	case LED_OP_BLINK:
		if (dot->led_pwm) {
		#ifdef CONFIG_PWM
			chan = get_chan(dot->led_pin);
			pwm_pin_set_cycles(dot->op_dev, chan, dot->pixel_value.period, dot->pixel_value.pulse, pixel.start_state);
			//pwm_pin_set_usec(dot->op_dev, dot->led_pwm, dot->pixel_value.period * 1000, dot->pixel_value.pulse * 1000, pixel.start_state);
		#endif
		}
	}
#endif
	return 0;
}

