 /*
  * Copyright (c) 2019 Actions Semi Co., Ltd.
  *
  * SPDX-License-Identifier: Apache-2.0
  *
  * @file 
  * @brief 
  *
  * Change log:
  * 
  */

#ifndef _NV_BLE_STREAM_H
#define _NV_BLE_STREAM_H

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

/**bt manager nv ble strea init param*/
struct nv_ble_stream_init_param {
	void *gatt_attr;
	u8_t attr_size;
	void *req_write_attr;
	void *lwo_write_attr;
	void *cid_write_attr;
	void *auth_write_attr;
	void *notify_ccc_attr;
	void *connect_cb;
	int32_t read_timeout;
	int32_t write_timeout;
};


void nv_tx_attr_set(void *tx_attr, void *ccc_attr);
io_stream_t nv_ble_stream_create(void *param);
#endif	/* _NV_BLE_STREAM_H */

