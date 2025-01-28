/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt pbap interface
 */

#define SYS_LOG_DOMAIN "btif_map"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

int btif_map_register_processer(void)
{
	return btsrv_register_msg_processer(MSG_BTSRV_MAP, &btsrv_map_process);
}

int btif_map_client_connect(struct bt_map_connect_param *param)
{
	if (!param || !param->app_id || !param->cb) {
		return -EINVAL;
	}

	return btsrv_function_call_malloc(MSG_BTSRV_MAP, MSG_BTSRV_MAP_CONNECT, (void *)param, sizeof(struct bt_map_connect_param), 0);

}

int btif_map_client_set_folder(struct bt_map_set_folder_param *param)
{
	if (!param || !param->app_id) {
		return -EINVAL;
	}

	return btsrv_function_call_malloc(MSG_BTSRV_MAP, MSG_BTSRV_MAP_SET_FOLDER, (void *)param, sizeof(struct bt_map_set_folder_param), 0);

}

int btif_map_get_folder_listing(uint8_t app_id)
{
	int id = app_id;

	if (!id) {
		return -EINVAL;
	}

	return btsrv_function_call(MSG_BTSRV_MAP, MSG_BTSRV_MAP_GET_FOLDERLISTING, (void *)id);
}

int btif_map_get_messages_listing(struct bt_map_get_messages_listing_param *param)
{
	if (!param || !param->app_id) {
		return -EINVAL;
	}

	return btsrv_function_call_malloc(MSG_BTSRV_MAP, MSG_BTSRV_MAP_GET_MESSAGESLISTING, (void *)param, sizeof(struct bt_map_get_messages_listing_param), 0);
}

int btif_map_get_message(struct bt_map_get_param *param)
{
	if (!param || !param->app_id ||
		!param->cb || !param->map_path) {
		return -EINVAL;
	}

	return btsrv_function_call_malloc(MSG_BTSRV_MAP, MSG_BTSRV_MAP_GET_MESSAGE, (void *)param, sizeof(struct bt_map_get_param), 0);

}

int btif_map_abort_get(uint8_t app_id)
{
	int id = app_id;

	if (id == 0) {
		return -EINVAL;
	}

	return btsrv_function_call(MSG_BTSRV_MAP, MSG_BTSRV_MAP_ABORT_GET, (void *)id);
}

int btif_map_client_disconnect(uint8_t app_id)
{
	int id = app_id;

	if (id == 0) {
		return -EINVAL;
	}

	return btsrv_function_call(MSG_BTSRV_MAP, MSG_BTSRV_MAP_DISCONNECT, (void *)id);
}

