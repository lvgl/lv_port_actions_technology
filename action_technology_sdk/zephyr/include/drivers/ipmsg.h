/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_IPMSG_H_
#define ZEPHYR_INCLUDE_DRIVERS_IPMSG_H_

/**
 * @brief inter-processor message communication API.
 * @defgroup ipmsg_interface IPMSG Interface
 * @ingroup io_interfaces
 * @{
 */

#include <kernel.h>
#include <device.h>
#include <rbuf/rbuf_msg.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	IPMSG_BTC_IRQ,
	IPMSG_TWS0_IRQ,
	IPMSG_TWS1_IRQ,
	IPMSG_REG_PW_CTRL,
};

typedef struct {
	uint8_t set_hosc_cap:1;
	uint8_t set_max_rf_power:1;
	uint8_t set_ble_rf_power:1;
	uint8_t hosc_capacity;
	uint8_t bt_max_rf_tx_power;
	uint8_t ble_rf_tx_power;
} ipmsg_btc_init_param_t;

/**
 * @typedef ipmsg init parameter
 * @brief Set init parameter.
 *
 * @param param: Struct of ipmsg_btc_init_param_t
 */
typedef void (*ipmsg_init_param_t)(struct device *dev, void *param);

/**
 * @typedef ipmsg_load_t
 * @brief Callback API to load executable for the remote processor.
 *
 * See @a ipmsg_load() for argument definitions.
 */
typedef int (*ipmsg_load_t)(struct device *ipmdev, void *data, uint32_t size);

/**
 * @typedef ipmsg_start_t
 * @brief Callback API to start the remote processor.
 *
 * See @a ipmsg_start() for argument definitions.
 */
typedef int (*ipmsg_start_t)(struct device *ipmdev, uint32_t *send_id, uint32_t *recv_id);

/**
 * @typedef ipmsg_stop_t
 * @brief Callback API to stop the remote processor.
 *
 * See @a ipmsg_stop() for argument definitions.
 */
typedef int (*ipmsg_stop_t)(struct device *ipmdev);

/**
 * @typedef ipmsg_notify_t
 * @brief Callback API to notify the remote processor.
 *
 * See @a ipmsg_notify() for argument definitions.
 */
typedef int (*ipmsg_notify_t)(struct device *ipmdev);

/**
 * @typedef ipmsg_callback_t
 * @brief Callback API for incoming IPMSG messages
 *
 * These callbacks execute in interrupt context. Therefore, use only
 * interrupt-safe APIS. Registration of callbacks is done via
 * @a ipmsg_register_callback
 *
 * @param "void *context" Arbitrary context pointer provided at
 *        registration time.
 * @param "void *arg" Other arguments.
 * must be inferred using the message id/upper level protocol.
 */
typedef void (*ipmsg_callback_t)(void *context, void *arg);

/**
 * @typedef ipmsg_pm_ctrl_callback_t
 * @brief register power controler callback
 */
typedef void (*ipmsg_pm_ctrl_callback_t)(uint32_t command, uint32_t state);

/**
 * @typedef ipmsg_register_callback_t
 * @brief Callback API upon registration
 *
 * See @a ipmsg_register_callback() for argument definitions.
 */
typedef void (*ipmsg_register_callback_t)(struct device *ipmdev,
					ipmsg_callback_t cb, void *cb_context);

typedef void (*ipmsg_irq_enable_t)(struct device *dev, uint8_t irq);

typedef void (*ipmsg_irq_disable_t)(struct device *dev, uint8_t irq);

/**
 * @typedef ipmsg_handler_t
 * @brief Callback API upon registration
 *
 * See @a ipmsg_recv() for argument definitions.
 */
typedef int (*ipmsg_handler_t)(void *context, void *data, unsigned int size);

struct ipmsg_driver_api {
	ipmsg_init_param_t init_param;
	ipmsg_load_t load;
	ipmsg_start_t start;
	ipmsg_stop_t stop;
	ipmsg_notify_t notify;
	ipmsg_register_callback_t register_callback;
	ipmsg_irq_enable_t enable;
	ipmsg_irq_disable_t disable;
};

/**
 * @typedef ipmsg init parameter
 * @brief Set init parameter.
 *
 * @param param: Struct of ipmsg_btc_init_param_t
 */
static inline void ipmsg_init_param(struct device *ipmdev, void *param)
{
	const struct ipmsg_driver_api *api =
		(const struct ipmsg_driver_api *)ipmdev->api;

	api->init_param(ipmdev, param);
}

/**
 * @brief Load executable for the remote processor.
 * 
 * @param ipmdev Driver instance
 * @param data Executable data.
 * @param size Size of executable.
 *
 * @retval -1   Load failed.
 * @retval 0    On success.
 */
static inline int ipmsg_load(struct device *ipmdev, void *data, uint32_t size)
{
	const struct ipmsg_driver_api *api =
		(const struct ipmsg_driver_api *)ipmdev->api;

	return api->load(ipmdev, data, size);
}

/**
 * @brief Start the remote processor.
 * 
 * It assumes the firmware is already loaded.
 *
 * @param ipmdev Driver instance
 * @param send_id Message queue id for sending.
 * @param recv_id Message queue id for receiving.
 *
 * @retval -1   Stop failed.
 * @retval 0    On success.
 */
static inline int ipmsg_start(struct device *ipmdev, uint32_t *send_id, uint32_t *recv_id)
{
	const struct ipmsg_driver_api *api =
		(const struct ipmsg_driver_api *)ipmdev->api;

	return api->start(ipmdev, send_id, recv_id);
}

/**
 * @brief Stop the remote processor.
 *
 * @param ipmdev Driver instance
 *
 * @retval -1   Stop failed.
 * @retval 0    On success.
 */
static inline int ipmsg_stop(struct device *ipmdev)
{
	const struct ipmsg_driver_api *api =
		(const struct ipmsg_driver_api *)ipmdev->api;

	return api->stop(ipmdev);
}

/**
 * @brief Notify the remote processor.
 * 
 * This function notify the remote processor to receive message.
 *
 * @param ipmdev Driver instance
 *
 * @retval -1   Notify failed.
 * @retval 0    On success.
 */
static inline int ipmsg_notify(struct device *ipmdev)
{
	const struct ipmsg_driver_api *api =
		(const struct ipmsg_driver_api *)ipmdev->api;

	return api->notify(ipmdev);
}

/**
 * @brief Register a callback function for incoming messages.
 *
 * @param ipmdev Driver instance pointer.
 * @param id Message queue id.
 * @param cb Callback function to execute on incoming message interrupts.
 * @param context Application-specific context pointer which will be passed
 *        to the callback function when executed.
 */
static inline void ipmsg_register_callback(struct device *ipmdev,
					 ipmsg_callback_t cb, void *context)
{
	const struct ipmsg_driver_api *api =
		(const struct ipmsg_driver_api *)ipmdev->api;

	api->register_callback(ipmdev, cb, context);
}

static inline void ipmsg_tws_irq_enable(struct device *ipmdev, uint8_t irq)
{
	const struct ipmsg_driver_api *api =
		(const struct ipmsg_driver_api *)ipmdev->api;

	api->enable(ipmdev, irq);
}

static inline void ipmsg_tws_irq_disable(struct device *ipmdev, uint8_t irq)
{
	const struct ipmsg_driver_api *api =
		(const struct ipmsg_driver_api *)ipmdev->api;

	api->disable(ipmdev, irq);
}

/**
 * @brief Create message queue.
 *
 * @param t MSG or RAW.
 * @param s Size of queue.
 *
 * @retval id	Message queue id.
 */
#define ipmsg_create(type,size)		RBUF_TO_OF(rbuf_pool_alloc(RB_GET_POOL,size,type))

/**
 * @brief Destroy message queue.
 *
 * @param id Message queue id.
 *
 * @retval -1   Destroy failed.
 * @retval 0    On success.
 */
#define ipmsg_destroy(id)			rbuf_pool_free(RBUF_FR_OF(id))

/**
 * @brief Get message buffer before filling message.
 *
 * @param id Message queue id.
 * @param size Size of data.
 * @param psz Size of allocated buffer.
 *
 * @retval NULL     Get message buffer failed.
 * @retval other    On success.
 */
#define ipmsg_claim(id,size,psz)	rbuf_put_claim(RBUF_FR_OF(id),size,psz)

/**
 * @brief Send message buffer after filling message.
 *
 * @param id Message queue id.
 * @param size Size of requested data.
 *
 * @retval -1   Send message failed.
 * @retval 0    On success.
 */
#define ipmsg_send(id,size)			rbuf_put_finish(RBUF_FR_OF(id),size)

/**
 * @brief Determine if message queue has data.
 *
 * @param id Message queue id.
 *
 * @return 1 if has data, or 0 if not.
 */
#define ipmsg_pending(id)			(id > 0) && !rbuf_is_empty(RBUF_FR_OF(id))

/**
 * @brief Receive message buffer by handler.
 *
 * @param id Message queue id.
 * @param size Size of requested data.
 * @param hdl Handler for processing data.
 * @param ctx Contex for handler.
 *
 * @retval -1   Send message failed.
 * @retval 0    On success.
 */
#define ipmsg_recv(id,size,hdl,ctx)		rbuf_get_hdl(RBUF_FR_OF(id),size,hdl,ctx)

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_IPMSG_H_ */
