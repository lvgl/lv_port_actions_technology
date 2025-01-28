/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief btsrv storage
 */

#define SYS_LOG_DOMAIN "btsrv_storage"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

static void stack_property_flush_req(char *key)
{
	btsrv_event_notify(MSG_BTSRV_BASE, MSG_BTSRV_REQ_FLUSH_NVRAM, (void *)key);
}

int btsrv_storage_init(void)
{
	return hostif_reg_flush_nvram_cb(stack_property_flush_req);
}

int btsrv_storage_get_addr_linkkey(bd_address_t *addr, uint8_t *linkkey)
{
	return hostif_bt_store_find_linkkey((const bt_addr_t *)addr, linkkey);
}

int btsrv_storage_get_linkkey(struct bt_linkkey_info *info, uint8_t cnt)
{
	return hostif_bt_store_get_linkkey((struct br_linkkey_info *)info, cnt);
}

int btsrv_storage_update_linkkey(struct bt_linkkey_info *info, uint8_t cnt)
{
	return hostif_bt_store_update_linkkey((struct br_linkkey_info *)info, cnt);
}

int btsrv_storage_write_ori_linkkey(bd_address_t *addr, uint8_t *link_key)
{
	hostif_bt_store_write_ori_linkkey((bt_addr_t *)addr, link_key);
	return 0;
}

void btsrv_storage_clean_linkkey(bd_address_t *addr)
{
	hostif_bt_store_clear_linkkey((bt_addr_t *)addr);
}

