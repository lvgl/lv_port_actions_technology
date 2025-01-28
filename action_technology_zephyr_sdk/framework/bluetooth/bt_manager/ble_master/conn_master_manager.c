#include <os_common_api.h>
#include <acts_bluetooth/host_interface.h>
#include "conn_master_manager.h"

static K_MUTEX_DEFINE(ble_conn_lock);
static struct ble_connection le_master_conns[CONN_NUM];
static u8_t conn_num = 0;

void conn_manager_init(void)
{
	k_mutex_lock(&ble_conn_lock, K_FOREVER);
	memset(le_master_conns, 0, sizeof(le_master_conns));
	k_mutex_unlock(&ble_conn_lock);
}

struct ble_connection *ble_conn_get_by_index(int i)
{
	if (i < CONN_NUM) {
		return &le_master_conns[i];
	}

	return NULL;
}

static struct ble_connection *ble_conn_get(void)
{
	int i;

	k_mutex_lock(&ble_conn_lock, K_FOREVER);
	for (i = 0; i < CONN_NUM; i++) {
		if (le_master_conns[i].state == BLE_CONN_IDLE) {
			le_master_conns[i].state = BLE_CONN_GET_USED;
			k_mutex_unlock(&ble_conn_lock);
			return &le_master_conns[i];
		}
	}

	k_mutex_unlock(&ble_conn_lock);
	return NULL;
}

static void ble_conn_put(struct ble_connection *conn)
{
	k_mutex_lock(&ble_conn_lock, K_FOREVER);
	memset(conn, 0, sizeof(struct ble_connection));
	k_mutex_unlock(&ble_conn_lock);
}

bool conn_have_value_conn(void)
{
	int i;

	k_mutex_lock(&ble_conn_lock, K_FOREVER);
	for (i = 0; i < CONN_NUM; i++) {
		if (le_master_conns[i].state == BLE_CONN_IDLE) {
			k_mutex_unlock(&ble_conn_lock);
			return true;
		}
	}

	k_mutex_unlock(&ble_conn_lock);
	return false;
}

int conn_alloc(struct bt_conn *conn, u8_t state)
{
	int i;
	bool found = false;
	struct ble_connection *ble_conn = NULL;

	for (i = 0; i < CONN_NUM; i++) {
		SYS_LOG_INF("le_master_conns[%d].conn_index:%d", i, le_master_conns[i].conn_index);

		if (!bt_addr_le_cmp(&le_master_conns[i].addr, bt_conn_get_dst(conn))) {
			ble_conn = &le_master_conns[i];
			conn_num=i+1;//add by maximus fix 0730
			found = true;
			SYS_LOG_INF("conn_alloc: device found");
			break;
		}
	}

	if (!ble_conn) {
		for(i = 0; i < CONN_NUM; i++)
		{
			if(le_master_conns[i].conn_index == 0)
			{
				conn_num = i;
				break;
			}
		}
		conn_num++;
		SYS_LOG_INF("conn_alloc: new device, conn_num:%d",conn_num);
		ble_conn = ble_conn_get();
	}
	SYS_LOG_INF("fun: %s, conn_num: %d",__func__,conn_num);
	if (ble_conn) {
		ble_conn->conn = conn;
		ble_conn->state = state;
		ble_conn->auto_conn = false;
		ble_conn->conn_index = conn_num;

		if (!found) {
			bt_addr_le_copy(&ble_conn->addr, bt_conn_get_dst(conn));
		}
		return 0;
	}

	return -1;
}

struct ble_connection *ble_conn_find(struct bt_conn *conn)
{
	int i;

	k_mutex_lock(&ble_conn_lock, K_FOREVER);
	for (i = 0; i < CONN_NUM; i++) {
		if (le_master_conns[i].conn == conn) {
			k_mutex_unlock(&ble_conn_lock);
			return &le_master_conns[i];
		}
	}

	k_mutex_unlock(&ble_conn_lock);
	return NULL;
}

struct bt_conn *find_ble_conn_use_addr(const bt_addr_le_t *addr)
{
	int i;
	const bt_addr_le_t *ble_addr = addr;

	k_mutex_lock(&ble_conn_lock, K_FOREVER);
	for (i = 0; i < CONN_NUM; i++) {
		if (!memcmp(&le_master_conns[i].addr, ble_addr, sizeof(bt_addr_le_t))) {
			SYS_LOG_INF(" find_ble_conn_use_addr .find the ble conn");
			k_mutex_unlock(&ble_conn_lock);
			return le_master_conns[i].conn;
		}
	}

	k_mutex_unlock(&ble_conn_lock);
	return NULL;
}


struct ble_connection *find_ble_connection_use_addr(const bt_addr_le_t *addr)
{
	int i;
	const bt_addr_le_t *ble_addr = addr;

	k_mutex_lock(&ble_conn_lock, K_FOREVER);
	for (i = 0; i < CONN_NUM; i++) {
		if (!memcmp(&le_master_conns[i].addr.a, &ble_addr->a, sizeof(bt_addr_t))) {
			SYS_LOG_INF("find the ble conn");
			k_mutex_unlock(&ble_conn_lock);
			return &le_master_conns[i];
		}
	}

	k_mutex_unlock(&ble_conn_lock);
	return NULL;
}

void conn_release(struct bt_conn *conn)
{
	struct ble_connection *ble_conn = ble_conn_find(conn);

	if (ble_conn) {
		ble_conn_put(ble_conn);
	}
}

u8_t conn_master_get_state_by_addr(const bt_addr_le_t *addr)
{
	int i;

	k_mutex_lock(&ble_conn_lock, K_FOREVER);
	for (i = 0; i < CONN_NUM; i++) {
		if (!bt_addr_le_cmp(&le_master_conns[i].addr, addr)) {
			k_mutex_unlock(&ble_conn_lock);
			return le_master_conns[i].state;
		}
	}

	k_mutex_unlock(&ble_conn_lock);
	return BLE_CONN_IDLE;
}

int conn_master_get_state(struct bt_conn *conn)
{
	struct ble_connection *ble_conn = ble_conn_find(conn);
	if (ble_conn) {
		return ble_conn->state;
	}

	return 0;
}

int conn_master_update_state(struct bt_conn *conn, u8_t state)
{
	struct ble_connection *ble_conn = ble_conn_find(conn);
	if (ble_conn) {
		ble_conn->state = state;
		return 0;
	}

	return -1;
}

int conn_master_add_test_write_hdl(struct bt_conn *conn, uint16_t hdl)
{
	struct ble_connection *ble_dev = ble_conn_find(conn);

	if (ble_dev == NULL) {
		return -EIO;
	}

	ble_dev->test_write_params.handle = hdl;
	return 0;
}

void ble_master_dump_conn_info(void)
{
	int i;
	char addr[BT_ADDR_LE_STR_LEN];

	k_mutex_lock(&ble_conn_lock, K_FOREVER);
	printk("Ble master info\n");

	for (i = 0; i < CONN_NUM; i++) {
		if (le_master_conns[i].conn) {
			bt_addr_le_to_str(bt_conn_get_dst(le_master_conns[i].conn), addr, sizeof(addr));
			printk("Conn hdl 0x%x %s\n", hostif_bt_conn_get_handle(le_master_conns[i].conn), addr);
			printk("\t State %d %d %d %d %d %d\n", le_master_conns[i].state, le_master_conns[i].pair_state,
							le_master_conns[i].auto_conn, le_master_conns[i].conn_index,
							le_master_conns[i].sd_index, le_master_conns[i].test_write_wait_finish);
		}
	}

	k_mutex_unlock(&ble_conn_lock);
}
