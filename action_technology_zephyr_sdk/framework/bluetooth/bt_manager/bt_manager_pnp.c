/*
 * Copyright (c) 2019 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt manager hfp ag profile.
 */
#define SYS_LOG_NO_NEWLINE
#define SYS_LOG_DOMAIN "bt manager"

#include <os_common_api.h>
#include <media_type.h>
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <bt_manager.h>
#include <power_manager.h>
#include <app_manager.h>
#include <sys_event.h>
#include <mem_manager.h>
#include "bt_manager_inner.h"
#include <assert.h>
#include "btservice_api.h"

struct bt_manager_pnp_info_t {
	uint16_t vendor_id;
	uint16_t product_id;
};

static struct bt_manager_pnp_info_t pnp_info_manager;

static void _bt_manager_pnp_info_callback(btsrv_pnp_info_event_e event, void *param, int param_size)
{
	uint16_t *value = param;

	switch (event) {
	case BTSRV_PNP_VENDOR_ID:
	{
		if (sizeof(uint16_t) != param_size) {
			SYS_LOG_ERR("param_size 0x%x\n",param_size);
			break;
		}

		pnp_info_manager.vendor_id = (*value);
		SYS_LOG_INF("vendor id 0x%x\n",pnp_info_manager.vendor_id);
#ifdef CONFIG_BR_SDP_ACTIVE_REGISTER
#ifdef CONFIG_GATT_OVER_BREDR
		/**XIAOMI phone use gatt over edr.**/
		if (0x38F != pnp_info_manager.vendor_id) {

			//void bt_unregister_gobr_sdp(void);
			//bt_unregister_gobr_sdp();
		}

		/**Avoid  connect over LE and EDR together.**/
		if (0x38F == pnp_info_manager.vendor_id) {
			//bt_manager_ble_disconnect();
		}
#endif
#endif
		break;
	}
	case BTSRV_PNP_PRODUCT_ID:
	{
		if (sizeof(uint16_t) != param_size) {
			SYS_LOG_ERR("param_size 0x%x\n",param_size);
			break;
		}

		pnp_info_manager.product_id = (*value);
		SYS_LOG_INF("product_id 0x%x\n",pnp_info_manager.product_id);
		break;
	}
	default:
		break;
	}
}

int bt_manager_pnp_info_search_init(void)
{
	memset(&pnp_info_manager, 0, sizeof(struct bt_manager_pnp_info_t));
	btif_pnp_info_search_start(&_bt_manager_pnp_info_callback);
	return 0;
}

int bt_manager_pnp_info_search_deinit(void)
{
	return btif_pnp_info_search_stop();
}
