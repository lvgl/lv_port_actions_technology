
/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file hotplug manager interface
 * @brief manager all hotplug devive, include hotplug detected,
 *  state report , and get hotplug state. 
 */

#ifndef __HOTPLUG_MANAGER_H__
#define __HOTPLUG_MANAGER_H__
//#include <hotplug.h>

/**
 * @defgroup hotplug_manager_apis App hot plug Manager APIs
 * @ingroup system_apis
 * @{
 */

/** hotplug state enum */
typedef enum
{
	/** hotplug state not changed */
	HOTPLUG_NONE,
	/** hotplug state plug in*/
	HOTPLUG_IN,
	/** hotplug state plug out*/
	HOTPLUG_OUT,
}hotplug_state_e;

/** hotplug device type enum */
typedef enum
{
	/** hotplug type linein*/
	HOTPLUG_LINEIN = 1,
	/** hotplug type sdcard*/
	HOTPLUG_SDCARD,
	/** hotplug type usb device*/
	HOTPLUG_USB_DEVICE,
	/** hotplug type usb host*/
	HOTPLUG_USB_HOST,
	/** hotplug type usb stub*/
	HOTPLUG_USB_STUB,
	/** hotplug type charger*/
	HOTPLUG_CHARGER,
}hotplug_type_e;

/**
 * @brief hotplug manager init function
 * @details init hotplug manager struct and init hotplug device 
 *  add monitor work to system monitor
 *
 * @return 0 success 
 * @return others failed
 */
int hotplug_manager_init(void);

/**
 * @brief hotplug manager get device state
 *
 * @param hotplug_device_type hotplug type of device.
 *
 * @return hotplug state of target device
 */
int hotplug_manager_get_state(int hotplug_device_type);

/**
 * @cond INTERNAL_HIDDEN
 */

/** @def MAX_HOTPLUG_DEVICE_NUM
 *
 *  @brief support max hotplug device number
 *
 *  @details max hotplug device number , if user want support much more devices
 *  must modify this macros
 */
#define MAX_HOTPLUG_DEVICE_NUM 5

typedef int (*hotplug_device_get_type)(void);

/** @def hotplug_device_get_state
 *
 *  @brief type of function pointer of device get state
 *
 *  @details this function return device state 
 *  @return hotplug_state_e
 */
typedef int (*hotplug_device_get_state)(void);

/** @def hotplug_device_hotplug_detect
 *
 *  @brief type of function pointer of device hotplug detect
 *
 *  @details this function return device hotplug state changed
 *  @return hotplug_state_e
 */
typedef int (*hotplug_device_hotplug_detect)(void);

/** @def hotplug_device_hotplug_fs_process
 *
 *  @brief type of function pointer of device hot plug fs process
 *
 *  @details this function do fs process accodrding device state 
 *  such as: mount fs when storage plugin and unmount fs when storage plug out
 *  @return 0 int success
 *  @others failed
 *  
 */
typedef int (*hotplug_device_hotplug_fs_process)(int device_state);

/** hotpulg device structure */
struct hotplug_device_t {
	/**type of device type @hotplug_type_e */
	uint8_t type;
	hotplug_device_get_type get_type;
	/**function pointer of get state ,plug in or plug out*/
	hotplug_device_get_state get_state;
	/**function pointer of detect */
	hotplug_device_hotplug_detect hotplug_detect;
	/**function pointer of fs prorss when device hotplug in/ hotplug out */
	hotplug_device_hotplug_fs_process fs_process;
};

struct hotplug_manager_context_t {
	uint8_t device_num;
	const struct hotplug_device_t *device[MAX_HOTPLUG_DEVICE_NUM];
};

int hotplug_linein_init(void);
int	hotplug_sdcard_init(void);
int	hotplug_usb_init(void);
uint8_t usb_hotplug_get_otgstate(void);
bool usb_hotplug_device_mode(void);
int hotplug_charger_init(void);
/**
 * @brief register hotplug device
 *
 * @param device register device 
 *
 * @return 0 success 
 * @return others failed
 */

int hotplug_device_register(const struct hotplug_device_t *device);

/**
 * @brief unregister hotplug device
 *
 * @param device unregister device 
 *
 * @return 0 success 
 * @return others failed
 */

int hotplug_device_unregister(const struct hotplug_device_t *device);
/**
 * INTERNAL_HIDDEN @endcond
 */
/**
 * @} end defgroup hotplug_manager_apis
 */
#endif
