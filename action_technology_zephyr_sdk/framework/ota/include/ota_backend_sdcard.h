/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA SDCARD backend interface
 */

#ifndef __OTA_BACKEND_SDCARD_H__
#define __OTA_BACKEND_SDCARD_H__

#include <ota_backend.h>

/** structure of ota backend sdcard init param, Initialized by the user*/
struct ota_backend_sdcard_init_param {
	/** ota.bin full path*/
	const char *fpath;
};

/**
 * @brief ota backend sdcard init.
 *
 * This routine init backend sdcard,calls by ota app.
 *
 * @param cb call back function,to tell ota app start stop upgrade,and upgrade progress.
 * @param param backend sdcard init param.
 *
 *return backend if init success.
 *return NULL if init fail.
 */

struct ota_backend *ota_backend_sdcard_init(ota_backend_notify_cb_t cb,
		struct ota_backend_sdcard_init_param *param);

#endif /* __OTA_BACKEND_SDCARD_H__ */
