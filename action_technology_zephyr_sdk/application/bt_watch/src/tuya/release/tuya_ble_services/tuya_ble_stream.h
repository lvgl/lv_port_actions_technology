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

#ifndef _TUYA_BLE_STREAM_H
#define _TUYA_BLE_STREAM_H

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

/**bt manager tuya ble strea init param*/
struct tuya_ble_stream_init_param {
	void *gatt_attr;
	u8_t attr_size;
	void *write_norsp_attr;
	void *notify_chrc_attr;
	void *notify_attr;
	void *notify_ccc_attr;
	void *connect_cb;
	int32_t read_timeout;
	int32_t write_timeout;
};

io_stream_t tuya_ble_stream_create(void *param);
#endif	/* _TUYA_BLE_STREAM_H */

