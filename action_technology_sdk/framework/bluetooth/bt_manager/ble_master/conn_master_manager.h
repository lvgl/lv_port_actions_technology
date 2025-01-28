#ifndef __CONN_MANAGER_H_
#define __CONN_MANAGER_H_

#include <zephyr/types.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>

#define CONN_NUM				2   /* Connect slave max number */
#define BLE_MAX_SERVICE			10  /* Connect salve max servcie */
#define BLE_MAX_CHRC			30  /* Connect salve max chrc */

enum ccc_type {
	BLE_CCC_NULL,
	BLE_CCC_HID,
	BLE_CCC_VOICE,
	BLE_CCC_BP,
	BLE_CCC_BG_M,
	BLE_CCC_BG_R,
};

enum conn_state {
	BLE_CONN_IDLE,
	BLE_CONN_GET_USED,
	BLE_CONN_CONNECTED,
	BLE_CONN_DISCONNECTEDE,
	BLE_CONN_ERROR,
};

struct ble_gatt_chrc {
	u8_t properties;
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	} ch_uuid;
	u8_t ccc_type;
	u16_t chrc_handle;
	u16_t value_handle;
	u16_t ccc_handle;
	struct bt_gatt_subscribe_params subscribe;
};

struct ble_service {
	union {
		struct bt_uuid uuid;
		struct bt_uuid_16 u16;
		struct bt_uuid_128 u128;
	} u;
	u16_t start_handle;
	u16_t end_handle;
	struct ble_gatt_chrc chrc[BLE_MAX_CHRC];
};

struct ble_connection {
	struct bt_conn *conn;
	u8_t ble_device_type;
	bt_addr_le_t addr;
	u8_t state;
	u8_t pair_state;
	bool auto_conn;
	u8_t conn_index;
	u8_t sd_index;			/* service discover index */
	struct ble_service service[BLE_MAX_SERVICE];
	uint8_t test_write_wait_finish:1;
	struct bt_gatt_write_params test_write_params;		/* For test */
};

void conn_manager_init(void);
bool conn_have_value_conn(void);
int conn_alloc(struct bt_conn *conn, u8_t state);
void conn_release(struct bt_conn *conn);
struct ble_connection *ble_conn_find(struct bt_conn *conn);
struct bt_conn *find_ble_conn_use_addr(const bt_addr_le_t *addr);
struct ble_connection *find_ble_connection_use_addr(const bt_addr_le_t *addr);
struct ble_connection *ble_conn_get_by_index(int i);

int conn_master_get_state(struct bt_conn *conn);
u8_t conn_master_get_state_by_addr(const bt_addr_le_t *addr);
int conn_master_update_state(struct bt_conn *conn, u8_t state);
int conn_master_add_test_write_hdl(struct bt_conn *conn, uint16_t hdl);

#endif /* __CONN_MANAGER_H_ */
