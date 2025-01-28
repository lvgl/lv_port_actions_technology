/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief OTA bluetooth backend interface
 */

#ifndef __OTA_TRANS_BT_H__
#define __OTA_TRANS_BT_H__

#include <stream.h>
//#include <types.h>

//#include <bt_eg_api.h>
#include <ota_trans.h>
#include <ota_api.h>
#include <crc.h>

#ifndef uint8
typedef unsigned char uint8;
#endif

#ifndef int8
typedef signed char int8;
#endif

#ifndef uint16
typedef unsigned short uint16;
#endif

#ifndef int16
typedef signed short int16;
#endif

#ifndef uint32
typedef unsigned int uint32;
#endif

#ifndef int32
typedef signed int int32;
#endif


struct ota_bt_dev{
    u8_t valid;  
    u8_t rssi;
    u8_t bd_addr[6];   
} ;

struct ota_trans_image_info{
    int image_offset;
    int read_len;
    int image_percent;
};

struct ota_trans_upgrade_image{
    u8_t *buffer;
    int size;
    u32_t crc;
	int ctn;
};

struct ota_trans_image_head_info{
    u32_t version;
    u32_t head_crc;
};


struct ota_trans_bt {
	struct ota_trans trans;
	struct cli_prot_context cli_ctx;
	struct ota_bt_dev ota_dev;
};

struct ota_trans_bt_init_param {
	const u8_t *spp_uuid;
	struct le_gatt_attr *le_gatt_attr;
	struct le_gatt_attr *tx_attr;
	struct le_gatt_attr *ccc_attr;
	struct le_gatt_attr *rx_attr;
};

struct ota_trans *ota_trans_bt_init(ota_trans_notify_cb_t cb,
					struct ota_trans_bt_init_param *param);
void ota_trans_bt_exit(struct ota_trans *trans);

#endif /* __OTA_BACKEND_BT_H__ */
