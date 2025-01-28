/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2CMT_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2CMT_H_

/**
 * @brief inter-processor message communication API.
 * @defgroup ipmsg_interface IPMSG Interface
 * @ingroup io_interfaces
 * @{
 */

#include <kernel.h>
#include <device.h>
#include <drivers/ppi.h>
#include <drivers/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
//constants
/******************************************************************************/
//transfer status
enum i2c_xfer_stat {
	I2C_XFER_OK = 0,			// no error
	I2C_XFER_BUS_ERR = -1,			// bus error
	I2C_XFER_NACK = -2,				// NACK
	I2C_XFER_NO_TCB = -3,			// transfer not completed
	I2C_XFER_START_FAILED = -4,// start failed
	I2C_XFER_STOP_FAILED = -5, // stop failed
};

//i2c task num
#define I2C_TASK_NUM			(4)

//i2c task DMA irq type
enum i2c_task_irq_type {
	I2C_TASK_IRQ_CMPLT = (1 << 0),
	I2C_TASK_IRQ_HALF_CMPLT = (1 << 1),
	I2C_TASK_IRQ_NACK = (1 << 2),
	I2C_TASK_IRQ_BUS_ERROR = (1 << 3),
};

/******************************************************************************/
//typedef
/******************************************************************************/
typedef struct i2c_xfer_s {
	unsigned short dev;
	unsigned short flag;
	unsigned char *cmd_buf;
	unsigned char *dat_buf;
	unsigned short cmd_len;
	unsigned short dat_len;
} i2c_xfer_t;

typedef struct i2c_task_ctl {
  unsigned int rwsel       : 1;   // read / write selection
  unsigned int sdevaddr    : 7;   // device address
  unsigned int sdataddr    : 8;   // data address
  unsigned int rdlen_wsdat : 12;  // read byte length or write single-byte data
  unsigned int tdataddr    : 1;   // ignore data address
  unsigned int swen        : 1;   // single write enable
  unsigned int abort       : 1;   // abort task
  unsigned int soft        : 1;   // software start 
} i2c_task_ctl_t;

typedef struct i2c_task_dma {
  unsigned int start   : 1;   // dma start/stop
  unsigned int reload  : 1;   // dma reload mode
  unsigned int addr;          // transfer address
  unsigned int len;           // transfer length
} i2c_task_dma_t;

typedef struct i2c_task {
	unsigned int irq_type; // irq enable
	i2c_task_ctl_t ctl;    // task ctl
	i2c_task_dma_t dma;    // task dma
	task_trig_t trig;      // task trig
} i2c_task_t;

/******************************************************************************/
//functions
/******************************************************************************/

/**
 * @typedef i2c_task_callback_t
 * @brief Callback API to handle spimt irq.
 */
typedef void (*i2c_task_callback_t) (unsigned char *buf, int len, void *context);

/**
 * @typedef i2c_register_callback_t
 * @brief Callback API upon registration
 *
 * See @a ipmsg_register_callback() for argument definitions.
 */
typedef void (*i2c_register_callback_t)(struct device *dev,
					int task_id, i2c_task_callback_t cb, void *cb_context);

/**
 * @typedef i2c_task_start_t
 * @brief Callback API to start spimt task.
 */
typedef int (*i2c_task_start_t) (struct device *dev, int task_id,
					const i2c_task_t *attr);

/**
 * @typedef i2c_task_stop_t
 * @brief Callback API to stop spimt task.
 */
typedef int (*i2c_task_stop_t) (struct device *dev, int task_id);

struct i2cmt_driver_api {
	struct i2c_driver_api i2c_api;
	i2c_register_callback_t register_callback;
	i2c_task_start_t task_start;
	i2c_task_stop_t task_stop;
};

/**
 * @brief Register a callback function for incoming messages.
 *
 * @param dev Driver instance pointer.
 * @param id Message queue id.
 * @param cb Callback function to execute on incoming message interrupts.
 * @param context Application-specific context pointer which will be passed
 *        to the callback function when executed.
 */
static inline void i2c_register_callback(struct device *dev,
					int task_id, i2c_task_callback_t cb, void *context)
{
	const struct i2cmt_driver_api *api =
		(const struct i2cmt_driver_api *)dev->api;

	api->register_callback(dev, task_id, cb, context);
}

/**
 * @brief Start task for spimt.
 * 
 * @param dev Driver instance
 * @param task_id Task number.
 * @param attr Task attribute.
 * @param hdl Task handler.
 *
 */
static inline int i2c_task_start(struct device *dev, int task_id, 
					const i2c_task_t *attr)
{
	const struct i2cmt_driver_api *api =
		(const struct i2cmt_driver_api *)dev->api;

	return api->task_start(dev, task_id, attr);
}

/**
 * @brief Stop task for i2cmt.
 * 
 * @param dev Driver instance
 * @param task_id Task number.
 *
 */
static inline int i2c_task_stop(struct device *dev, int task_id)
{
	const struct i2cmt_driver_api *api =
		(const struct i2cmt_driver_api *)dev->api;

	return api->task_stop(dev, task_id);
}

/**
 * @brief Get task data for i2cmt.
 * 
 * @param bus_id Bus No.
 * @param task_id Task number.
 * @param trig trigger source.
 * @param plen Data length pointer.
 *
 */
uint8_t* i2c_task_get_data(int bus_id, int task_id, int trig, int *plen);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2CMT_H_ */
