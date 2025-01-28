/*
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file input_dev.h
 * @brief input devices driver common interface
 *
 *          This file builds a public API for input KEY event(pressed or released) and KEY type
 *          to notify user by a callback function.
 */

#ifndef __INCLUDE_INPUT_DEV_H__
#define __INCLUDE_INPUT_DEV_H__

#include <stdint.h>
#include <device.h>

/**
 * @defgroup input_device_apis Driver Input Device APIs
 * @ingroup driver_apis
 * @{
 */

/**
 * @brief The <b> input devices </b> can be the following modules.
 *          - ADC KEY
 *          - ONOFF KEY
 *          - GPIO KEY
 *          - TP KEY
 *          - QD(quad decoder) KEY
 *          - IR(infrared rays) KEY
 *          - CAPTURE KEY
 *
 *          User need to enable a input device and register an notify callback function.
 *          And if there is any input events happened, user will get the input value from
 *          the notify callback function.
 *
 * Example:
 *
 * @code
 *
 *   #include <drivers/input/input_dev.h>
 *
 *   static void adckey_notify_cb (struct device *dev, struct input_value *val)
 *   {
 *         printk("input type:%d code:%d value:%d\n", val->keypad.type, val->keypad.code, val->keypad.value);
 *   }
 *
 *   // 1. Get a input device instance.
 *    const struct device *adckey_dev = device_get_binding(CONFIG_INPUT_DEV_ACTS_ADCKEY_NAME);
 *    if (!adckey_dev) {
 *        printk("failed to get adc key device\n");
 *        return -ENODEV;
 *    }
 *
 *    // 2. Register a notify function to the specified input device.
 *    input_dev_register_notify(dev, adckey_notify_cb);
 *
 *    // 3. Enable the specified input device.
 *    input_dev_enable(adckey_dev);
 *
 *    // 4. Disable the specified input device.
 *    input_dev_disable(adckey_dev)
 *
 * @endcode
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Macros to define the differenct KEY functions.
 * @{
 */
#define KEY_RESERVED			0
#define KEY_POWER           	1
#define KEY_PREVIOUSSONG    	2
#define KEY_NEXTSONG        	3
#define KEY_VOL          		4
#define KEY_VOLUMEUP        	5
#define KEY_VOLUMEDOWN      	6
#define KEY_MENU            	7
#define KEY_CONNECT         	8
#define KEY_TBD	            	9
#define KEY_MUTE            	10
#define KEY_PAUSE_AND_RESUME 	11
#define KEY_FOLDER_ADD			12
#define KEY_FOLDER_SUB			13
#define KEY_NEXT_VOLADD			14
#define KEY_PREV_VOLSUB			15
#define KEY_NUM0 				16
#define KEY_NUM1				17
#define KEY_NUM2				18
#define KEY_NUM3				19
#define KEY_NUM4				20
#define KEY_NUM5				21
#define KEY_NUM6				22
#define KEY_NUM7				23
#define KEY_NUM8				24
#define KEY_NUM9				25
#define KEY_CH_ADD				26
#define KEY_CH_SUB				27
#define KEY_PAUSE           	28
#define KEY_RESUME          	29
#define KEY_KONB_CLOCKWISE      30
#define KEY_KONB_ANTICLOCKWISE  31


#define KEY_F1              	40
#define KEY_F2              	41
#define KEY_F3             	 	42
#define KEY_F4              	43
#define KEY_F5              	44
#define KEY_F6              	45
#define KEY_ADFU            	46

/**
 *    @brief The macro to define the minimal interresting key.
 *    @note We avoid low common keys in module aliases so they don't get huge.
 */
#define KEY_MIN_INTERESTING	    KEY_MUTE

#define LINEIN_DETECT		    100

#define KEY_MAX			        0x2ff
#define KEY_CNT			        (KEY_MAX+1)

/** @} */

/**
 * @name The macros to define different events.
 * @{
 */
#define EV_SYN			        0x00
#define EV_KEY			        0x01
#define EV_REL			        0x02
#define EV_ABS			        0x03
#define EV_MSC			        0x04
#define EV_SW			        0x05
#define EV_SR			        0x06  /*!< Sliding rheostat */
#define EV_LED			        0x11
#define EV_SND			        0x12
#define EV_REP			        0x14
#define EV_FF			        0x15
#define EV_PWR			        0x16
#define EV_FF_STATUS	        0x17
#define EV_MAX			        0x1f
#define EV_CNT			        (EV_MAX+1)
/** @} */

/**
 * @name The macros to define different input device types.
 * @{
 */
#define INPUT_DEV_TYPE_KEYBOARD		1
#define INPUT_DEV_TYPE_TOUCHPAD		2
/** @} */

/**
 * @brief The structure to specify the input device type.
 */
struct input_dev_config {
	uint8_t type; /*!< input device type */
};

/**
 * @brief The sturcture to describe the physicical attributes
 *    such as touch coordinate points or key pad measure values.
 */
struct input_value {
	union {
		struct{
			int16_t loc_x; /*!< logical point x */
			int16_t loc_y; /*!< logical point y */
			uint16_t pessure_value; /*!< pessure messure value */
			uint16_t gesture; /*!< finger gesture  */
		} point; /*!< touch key point infomation */
		struct{
			uint16_t type; /*!< key pad type */
			uint16_t code; /*!< key magic code */
			uint32_t value; /*!< key value */
		} keypad; /*!< key pad infomation */
		struct{
			union {
				struct{
					uint16_t mode;/* protocol kind */
					uint32_t *data;/*!< ir data */
					uint16_t carry_rate;/* carry rate */
				}protocol;
				struct{
					uint16_t mode;/* protocol kind */
					uint16_t onoff;/* on or off */
					uint32_t *cmd;/* ir data */
				}data;
			};
		} ir; /*!< ir infomation */

	};
};

/**
 * @brief Input gesture types.
 *
 * Input gesture typesenumeration.
 */
enum {
	/** direction gestures: left, right, top, bottom */
	INPUT_GESTURE_DIRECTION = BIT(0),
};

/**
 * @brief Structure holding input device capabilities
 *    such as touch coordinate points or key pad measure values.
 */
struct input_capabilities {
	union {
		struct {
			uint32_t supported_gestures; /*!< finger direction gesture */
		} pointer; /*!< touch key pointer capabilities */
	};
};

/** @brief Callback function to notify different events which reported by input devices
 *
 *    @note User need to use #register_notify to register the callback function
 *       and use #unregister_notify to unregister.
 */
typedef void (*input_notify_t) (struct device *dev, struct input_value *val);

/**
 * @brief Input device driver public API
 *
 * @note Funtions to define a common behaviors of input devices.
 */
struct input_dev_driver_api {
	void (*enable)(const struct device *dev);
	void (*disable)(const struct device *dev);
	void (*inquiry)(const struct device *dev, struct input_value *val);
	void (*register_notify)(const struct device *dev, input_notify_t notify);
	void (*unregister_notify)(const struct device *dev, input_notify_t notify);
	void (*get_capabilities)(const struct device *dev, struct input_capabilities *capabilities);
};

/**
 * @brief Function to enable the specified input device.
 *
 * @param dev Pointer to the device structure for the input driver instance.
 *
 * @return none
 */
static inline void input_dev_enable(const struct device *dev)
{
	const struct input_dev_driver_api *api = dev->api;

	if (api->enable)
		api->enable(dev);
}

/**
 * @brief Function to disable the specified input device.
 *
 * @param dev Pointer to the device structure for the input driver instance.
 *
 * @return none
 */
static inline void input_dev_disable(const struct device *dev)
{
	const struct input_dev_driver_api *api = dev->api;

	if (api->disable)
		api->disable(dev);
}

/**
 * @brief Function to inquiry the current input status by the specified input device.
 *
 * @param dev Pointer to the device structure for the input driver instance.
 *
 * @return none
 */
static inline void input_dev_inquiry(const struct device *dev,
					     struct input_value *val)
{
	const struct input_dev_driver_api *api = dev->api;

	if (api->inquiry)
		api->inquiry(dev, val);
}

/**
 * @brief Function to register a notify callback function to the specified input device.
 *
 * @param dev Pointer to the device structure for the input driver instance.
 *
 * @param notify The notify callback function.
 *
 * @return none
 */
static inline void input_dev_register_notify(const struct device *dev,
					     input_notify_t notify)
{
	const struct input_dev_driver_api *api = dev->api;

	if (api->register_notify)
		api->register_notify(dev, notify);
}

/**
 * @brief Function to unregister a notify callback function to the specified input device.
 *
 * @param dev Pointer to the device structure for the input driver instance.
 *
 * @param notify The notify callback function.
 *
 * @return none
 */
static inline void input_dev_unregister_notify(const struct device *dev,
					       input_notify_t notify)
{
	const struct input_dev_driver_api *api = dev->api;

	if (api->unregister_notify)
		api->unregister_notify(dev, notify);
}

/**
 * @brief Get input device capabilities
 *
 * @param dev Pointer to device structure
 * @param capabilities Pointer to capabilities structure to populate
 *
 * @return none
 */
static inline void input_dev_get_capabilities(const struct device *dev,
					struct input_capabilities *capabilities)
{
	const struct input_dev_driver_api *api = dev->api;

	if (api->get_capabilities) {
		api->get_capabilities(dev, capabilities);
	}
}

#ifdef __cplusplus
}
#endif

/**
 * @} end defgroup input_device_apis
 */

#endif  /* __INCLUDE_INPUT_DEV_H__ */
