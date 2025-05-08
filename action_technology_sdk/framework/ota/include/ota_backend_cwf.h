/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA cloud watchface backend interface
 */

#ifndef __OTA_BACKEND_CWF_H__
#define __OTA_BACKEND_CWF_H__

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


/** structure of ota backend cwf init param, Initialized by the user*/
struct ota_backend_cwf_init_param {
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
 * @brief ota backend cwf init.
 *
 * This routine init backend bt,calls by ota app.
 *
 * @param cb call back function,to tell ota app start stop upgrade,and upgrade progress.
 * @param param backend bt init param.
 *
 *return backend if init success.
 *return NULL if init fail.
 */
struct ota_backend *ota_backend_cwf_init(ota_backend_notify_cb_t cb,
					struct ota_backend_cwf_init_param *param);

/**
 * @brief ota backend cwf exit.
 *
 * This routine free backend bt.
 *
 * @param backend pointer to backend
 *
 */
void ota_backend_cwf_exit(struct ota_backend *backend);

/**
 * @brief ota backend cwf get stream.
 *
 * This routine free backend bt.
 *
 * @param backend pointer to backend
 *return stream if init success.
 *
 */
io_stream_t ota_backend_cwf_get_stream(struct ota_backend *backend);

#endif /* __OTA_BACKEND_CWF_H__ */
