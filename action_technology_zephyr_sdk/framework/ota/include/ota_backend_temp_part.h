/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA Temp partition backend interface
 */

#ifndef __OTA_BACKEND_TEMP_PART_H__
#define __OTA_BACKEND_TEMP_PART_H__

#include <ota_backend.h>

/** structure of ota backend temp part init param, Initialized by the user*/
struct ota_backend_temp_part_init_param {
	/** device name */
	const char *dev_name;
};

/**
 * @brief ota backend temp part init.
 *
 * This routine init backend temp part,calls by ota app.
 *
 * @param cb call back function,to tell ota app start stop upgrade,and upgrade progress.
 * @param param backend temp part init param.
 *
 *return backend if init success.
 *return NULL if init fail.
 */

struct ota_backend *ota_backend_temp_part_init(ota_backend_notify_cb_t cb,
		struct ota_backend_temp_part_init_param *param);

#endif /* __OTA_BACKEND_TEMP_PART_H__ */
