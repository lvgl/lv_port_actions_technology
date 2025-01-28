/**
 * @file
 *
 * @brief Public APIs for the CEC drivers.
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CEC_H__
#define __CEC_H__

/**
 * @brief HDMI CEC Interface
 * @defgroup cec_interface CEC Interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif
#include <zephyr/types.h>
#include <device.h>

/*
 * The following #defines are used to configure the CEC controller.
 */
/** CEC transfer MAX size */
#define CEC_TRANSFER_MAX_SIZE					(16)

/**
 * @brief One CEC Message.
 *
 * This defines one CEC message to transact on the CEC bus.
 */
struct cec_msg {
	uint8_t buf[CEC_TRANSFER_MAX_SIZE]; /* Data buffer in bytes */
	uint8_t len; /* Valid length of buffer in bytes */
	uint8_t initiator : 4; /* The initiator address  */
	uint8_t destination : 4; /* The destination address  */
};

struct cec_driver_api {
	int (*config)(const struct device *dev, uint8_t local_addr);
	int (*write)(const struct device *dev, const struct cec_msg *msg, uint32_t timeout_ms);
	int (*read)(const struct device *dev, struct cec_msg *msg, uint32_t timeout_ms);
};

/**
 * @brief Configure the local address of CEC device.
 *
 * @param local_addr The CEC local address.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 */
static inline int cec_config_local_addr(const struct device *dev, uint8_t local_addr)
{
	const struct cec_driver_api *api = dev->api;

	return api->config(dev, local_addr);
}

/**
 * @brief Write a set amount of data to an CEC device.
 *
 * This routine writes a set amount of data synchronously.
 *
 * @param msg Pointer to the device structure for a CEC message.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int cec_write(const struct device *dev, const struct cec_msg *msg, uint32_t timeout_ms)
{
	const struct cec_driver_api *api = dev->api;
	return api->write(dev, msg, timeout_ms);
}

/**
 * @brief Read a set amount of data from an CEC device.
 *
 * This routine reads a set amount of data synchronously.
 *
 * @param msg Pointer to the device structure for a CEC message.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int cec_read(const struct device *dev, struct cec_msg *msg, uint32_t timeout_ms)
{
	const struct cec_driver_api *api = dev->api;
	return api->read(dev, msg, timeout_ms);
}

#ifdef __cplusplus
}
#endif

#endif /* __CEC_H__ */
