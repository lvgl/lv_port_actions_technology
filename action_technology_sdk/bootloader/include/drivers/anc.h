/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for ANC drivers and applications
 */

#ifndef __ANC_H__
#define __ANC_H__

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <zephyr/types.h>
#include <device.h>
#include <drivers/cfg_drv/dev_config.h>

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************/
/* anc image definition                                                           */
/**********************************************************************************/


#define ANC_IMAGE_NMLEN (12)

enum {
	ANC_STATUS_POWEROFF,
	ANC_STATUS_POWERON,
	ANC_STATUS_SUSPENDED,
};

enum {
	ANC_COMMAND_NONE,
	ANC_COMMAND_ANCTDATA,
	ANC_COMMAND_DUMPSTART,
	ANC_COMMAND_DUMPSTOP,
	ANC_COMMAND_SRCHANGE,
	ANC_COMMAND_POWEROFF,
	ANC_COMMAND_MAX,
};

struct anc_imageinfo {
	const char *name;
	const void *ptr;
	size_t size;
	uint32_t entry_point;
};

/* boot arguments of anc image */
struct anc_bootargs {
	uint32_t cmd_buf; /* address of command buffer */
	uint32_t debug_buf;	/*use for debug*/
	uint32_t audsp_buf;	/*communicate with audio dsp*/
	uint32_t ack_buf;	/*ack buf*/
};



/**********************************************************************************/
/* anc device driver api                                                          */
/**********************************************************************************/

/**
 * @cond INTERNAL_HIDDEN
 *
 * These are for internal use only, so skip these in public documentation.
 */

/**
 * @typedef anc_api_poweron
 * @brief Callback API for power on
 */
typedef int (*anc_api_poweron)(struct device *dev);

/**
 * @typedef anc_api_poweroff
 * @brief Callback API for power off
 */
typedef int (*anc_api_poweroff)(struct device *dev);


/**
 * @typedef anc_api_request_image
 * @brief Callback API for loading image
 */
typedef int (*anc_api_request_image)(struct device *dev, const struct anc_imageinfo *image);

/**
 * @typedef anc_api_release_image
 * @brief Callback API for releasing image
 */
typedef int (*anc_api_release_image)(struct device *dev);

/**
 * @typedef anc_api_release_mem
 * @brief Callback API for releasing image
 */
typedef int (*anc_api_request_mem)(struct device *dev, int type);

/**
 * @typedef anc_api_release_mem
 * @brief Callback API for releasing image
 */
typedef int (*anc_api_release_mem)(struct device *dev, int type);

/**
 * @typedef anc_api_get_status
 * @brief Callback API for get anc status
 */
typedef int (*anc_api_get_status)(struct device *dev);

/**
 * @typedef anc_api_send_command
 * @brief Callback API for send command to anc dsp
 */
typedef int (*anc_api_send_command)(struct device *dev, int type, void *data, int size);


struct anc_driver_api {
	anc_api_poweron poweron;
	anc_api_poweroff poweroff;
	anc_api_request_image request_image;
	anc_api_release_image release_image;
	anc_api_request_mem request_mem;
	anc_api_release_mem release_mem;
	anc_api_get_status get_status;
	anc_api_send_command send_command;
};

/**
 * @endcond
 */

/**
 * @brief start the anc device
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param cmdbuf  Address of session command buffer.
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int anc_poweron(struct device *dev)
{
	const struct anc_driver_api *api = dev->api;

	return api->poweron(dev);
}

/**
 * @brief stop the anc device
 *
 * @param dev     Pointer to the device structure for the driver instance.
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int anc_poweroff(struct device *dev)
{
	const struct anc_driver_api *api = dev->api;

	return api->poweroff(dev);
}

/**
 * @brief request the anc image
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param image anc image information
 * @param type anc image filetype
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int anc_request_image(struct device *dev, const struct anc_imageinfo *image)
{
    const struct anc_driver_api *api = dev->api;

    return api->request_image(dev, image);
}

/**
 * @brief release the anc image
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param type anc image filetype
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int anc_release_image(struct device *dev)
{
    const struct anc_driver_api *api = dev->api;

    return api->release_image(dev);
}

/**
 * @brief request the anc mem
 *
 * @param dev     Pointer to the device structure for the driver instance.
 * @param image anc image information
 * @param type anc image filetype
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int anc_request_mem(struct device *dev,int type)
{
    const struct anc_driver_api *api = dev->api;

    return api->request_mem(dev, type);
}

/**
 * @brief release the anc image
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param type anc image filetype
 *
 * @retval 0 if successful.
 * @retval Negative errno code if failure.
 */
static inline int anc_release_mem(struct device *dev, int type)
{
    const struct anc_driver_api *api = dev->api;

    return api->release_mem(dev, type);
}


/**
 * @brief get anc dsp status
 *
 * @param dev  Pointer to the device structure for the driver instance.
 *
 * @retval return anc dsp status
 */
static inline int anc_get_status(struct device *dev)
{
	const struct anc_driver_api *api = dev->api;

	return api->get_status(dev);
}


/**
 * @brief send data to anc dsp
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param type  data type
 * @param data data address
 * @param size data size
 * @retval 0 if send  data success
 * @retval -1 if send  data failed
 */
static inline int anc_send_command(struct device *dev, int type, void *data, int size)
{
	const struct anc_driver_api *api = dev->api;

	return api->send_command(dev, type, data, size);
}


/**
 * @brief send data to anc dsp
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param samplerate  samplerate
 * @param dac_digctl DAC_DIGCTL register value
 * @retval 0  success
 * @retval -1 failed
 */
static inline int anc_fs_change(struct device *dev, uint32_t samplerate, uint32_t dac_digctl)
{
	uint32_t data[2];
	const struct anc_driver_api *api = dev->api;

	data[0] = samplerate;
	data[1] = dac_digctl;
	return api->send_command(dev, ANC_COMMAND_SRCHANGE, data, 8);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __ANC_H__ */
