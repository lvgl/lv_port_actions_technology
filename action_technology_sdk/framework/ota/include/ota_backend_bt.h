/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA bluetooth backend interface
 */

#ifndef __OTA_BACKEND_BT_H__
#define __OTA_BACKEND_BT_H__

#include <stream.h>

#include <ota_backend.h>
typedef io_stream_t stream_cb(void *);

typedef int (*ota_backend_type_cb_t)(int);

/** enum, ota type*/
enum OTA_TYPE_FOR_DEVICE{
	PROT_OTA_PHONE_APP = 0,
	PROT_OTA_FACTORY_OFFLINE = 1,
	PROT_OTA_DONGLE_PC_CONDIF = 2,
};


/** structure of ota backend bt init param, Initialized by the user*/
struct ota_backend_bt_init_param {
	const uint8_t *spp_uuid;/**spp uuid*/
	void *gatt_attr;
	uint8_t attr_size;
	void *tx_chrc_attr;
	void *tx_attr;
	void *tx_ccc_attr;
	void *rx_attr;
	s32_t read_timeout;/** read data from bt time out*/
	s32_t write_timeout;/** send data to bt time out*/
};

/**
 * @brief ota backend bt init.
 *
 * This routine init backend bt,calls by ota app.
 *
 * @param cb call back function,to tell ota app start stop upgrade,and upgrade progress.
 * @param param backend bt init param.
 *
 *return backend if init success.
 *return NULL if init fail.
 */

struct ota_backend *ota_backend_bt_init(ota_backend_notify_cb_t cb,
					struct ota_backend_bt_init_param *param);

struct ota_backend *ota_backend_zble_init(ota_backend_notify_cb_t cb);

/**
 * @brief ota backend bt exit.
 *
 * This routine free backend bt.
 *
 * @param backend pointer to backend
 *
 */

void ota_backend_bt_exit(struct ota_backend *backend);

/**
 * @brief ota backend load bt init.
 *
 * This routine init backend load bt,calls by ota app.
 *
 * @param cb call back function,to tell ota app start stop upgrade,and upgrade progress.
 * @param param backend bt init param.
 * @param scb stream call back function.
 * @param *pexist_stream get steam address.
 *
 *return backend if init success.
 *return NULL if init fail.
 */

struct ota_backend *ota_backend_load_bt_init(ota_backend_notify_cb_t cb,
					struct ota_backend_bt_init_param *param, stream_cb scb, io_stream_t *pexist_stream);

/**
 * @brief ota backend stream set.
 *
 * This routine provides set dest stream to origin stream,calls by ota app.
 *
 * @param exist_stream handle of dest stream
 *
 */

void ota_backend_stream_set(io_stream_t exist_stream);

/**
 * @brief ota backend set callback for ota type.
 *
 * This routine provides set ota type callback to backend,calls by ota app.
 *
 * @param tcb pointer of callback
 *
 */
void ota_backend_ota_type_cb_set(ota_backend_type_cb_t tcb);

#endif /* __OTA_BACKEND_BT_H__ */
