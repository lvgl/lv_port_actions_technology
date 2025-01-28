#ifndef __BLE_MASTER_H__
#define __BLE_MASTER_H__

typedef void (*le_scan_discover_result_cb)(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,struct net_buf_simple *ad);

struct le_scan_param {
	le_scan_discover_result_cb cb;
};

void ble_master_init(void);

int le_master_scan_start(struct le_scan_param *le_scan_cb);

int le_master_scan_stop(void);

int le_master_dev_connect(bt_addr_le_t *addr);

int le_master_dev_disconnect(bt_addr_le_t *addr);

uint8_t le_master_get_scan_status(void);

void ble_master_dump_conn_info(void);

#endif
