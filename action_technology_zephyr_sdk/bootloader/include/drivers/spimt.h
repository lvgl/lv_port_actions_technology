/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SPIMT_H_
#define ZEPHYR_INCLUDE_DRIVERS_SPIMT_H_

/**
 * @brief inter-processor message communication API.
 * @defgroup ipmsg_interface IPMSG Interface
 * @ingroup io_interfaces
 * @{
 */

#include <kernel.h>
#include <device.h>
#include <drivers/ppi.h>
#include <drivers/spi.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
//constants
/******************************************************************************/

//spi task num
#define SPI_TASK_NUM			(8)

//spi task DMA irq type
enum spi_task_irq_type {
    SPI_TASK_IRQ_CMPLT = (1 << 0),
    SPI_TASK_IRQ_HALF_CMPLT = (1 << 1),
};

/******************************************************************************/
//typedef
/******************************************************************************/

typedef struct spi_task_ctl {
    unsigned int wsdat : 16;  // write data (single byte or half-word)
    unsigned int sbsh  : 1;   // length selection (single byte or half-word)
    unsigned int rwsel : 1;   // read / write selection
    unsigned int rdlen : 12;  // read byte length
    unsigned int rsvd  : 1;   // reserved
    unsigned int soft  : 1;   // software enable
} spi_task_ctl_t;

typedef struct spi_task_dma {
    unsigned int start   : 1;   // dma start/stop
    unsigned int reload  : 1;   // dma reload mode
    unsigned int addr;          // transfer address
    unsigned int len;           // transfer length
} spi_task_dma_t;

typedef struct spi_task {
    unsigned int irq_type; // irq enable
    spi_task_ctl_t ctl;    // task ctl
    spi_task_dma_t dma;    // task dma
    task_trig_t trig;      // task trig
} spi_task_t;

/******************************************************************************/
//functions
/******************************************************************************/

/**
 * @typedef spi_task_callback_t
 * @brief Callback API to handle spimt irq.
 */
typedef void (*spi_task_callback_t) (unsigned char *buf, int len, void *context);

/**
 * @typedef spi_register_callback_t
 * @brief Callback API upon registration
 *
 * See @a ipmsg_register_callback() for argument definitions.
 */
typedef void (*spi_register_callback_t)(const struct device *dev,
                    int task_id, spi_task_callback_t cb, void *cb_context);

/**
 * @typedef spi_task_start_t
 * @brief Callback API to start spimt task.
 */
typedef int (*spi_task_start_t) (const struct device *dev, int task_id,
                    const spi_task_t *attr);

/**
 * @typedef spi_task_stop_t
 * @brief Callback API to stop spimt task.
 */
typedef int (*spi_task_stop_t) (const struct device *dev, int task_id);

/**
 * @typedef spi_cs_select_t
 * @brief Callback API to select spimt cs.
 */
typedef int (*spi_cs_select_t) (const struct device *dev, int cs_num);

struct spimt_driver_api {
    struct spi_driver_api spi_api;
    spi_register_callback_t register_callback;
    spi_task_start_t task_start;
    spi_task_stop_t task_stop;
    spi_cs_select_t cs_select;
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
static inline void spi_register_callback(const struct device *dev,
                    int task_id, spi_task_callback_t cb, void *context)
{
    const struct spimt_driver_api *api =
        (const struct spimt_driver_api *)dev->api;

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
static inline int spi_task_start(const struct device *dev, int task_id,
                    const spi_task_t *attr)
{
    const struct spimt_driver_api *api =
        (const struct spimt_driver_api *)dev->api;

    return api->task_start(dev, task_id, attr);
}

/**
 * @brief Stop task for spimt.
 *
 * @param dev Driver instance
 * @param task_id Task number.
 *
 */
static inline int spi_task_stop(const struct device *dev, int task_id)
{
    const struct spimt_driver_api *api =
        (const struct spimt_driver_api *)dev->api;

    return api->task_stop(dev, task_id);
}

/**
 * @brief Select cs for spimt.
 *
 * @param dev Driver instance
 * @param cs_num Cs number.
 *
 */
static inline int spi_cs_select(const struct device *dev, int cs_num)
{
    const struct spimt_driver_api *api =
        (const struct spimt_driver_api *)dev->api;

    return api->cs_select(dev, cs_num);
}

/**
 * @brief Get task data for spimt.
 *
 * @param bus_id Bus No.
 * @param task_id Task number.
 * @param trig trigger source.
 * @param plen Data length pointer.
 *
 */
uint8_t* spi_task_get_data(int bus_id, int task_id, int trig, int *plen);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SPIMT_H_ */
