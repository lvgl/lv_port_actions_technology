/*
 *
 *
 */
	
#include "kernel.h"
#include "shell/shell.h"
#include "sys/printk.h"
#include "unistd.h"
#include <string.h>
#include <zephyr.h>
#include <spicache.h>
#include <device.h>
#include <sys_manager.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <posix/unistd.h>
#include <sys/fdtable.h>
#include <bt_manager.h>
#include <mem_manager.h>
#include <stream.h>
#include "mas.h"

#define MAS_BT_SEND_LEN_ONCE	(668)

struct rt_bluetooth_mgnt_device {
	uint8_t connect_type;
	io_stream_t sppble_stream;
	int sppble_stream_opened;
	mas_link_char *g_link_char;
	struct k_thread mas_rx_thread;
};
static struct rt_bluetooth_mgnt_device *bt_mgnt = NULL;
static K_KERNEL_STACK_DEFINE(mas_rx_stack, 2*1024);

/*****************************************************************/
static const uint8_t mas_spp_uuid[16] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, \
	0x00, 0x78, 0x69, 0x68, 0x70, 0x79, 0x6c, 0x67, 0x15, 0x12, 0x23};

#define BLE_MAS_SERVICE_UUID		BT_UUID_DECLARE_16(0xFFFF)
#define BLE_MAS_TX_UUID				BT_UUID_DECLARE_16(0xFF02)
#define BLE_MAS_RX_UUID				BT_UUID_DECLARE_16(0xFF03)

static struct bt_gatt_attr mas_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BLE_MAS_SERVICE_UUID),

	BT_GATT_CHARACTERISTIC(BLE_MAS_RX_UUID, BT_GATT_CHRC_WRITE_WITHOUT_RESP,
						BT_GATT_PERM_WRITE, NULL, NULL, NULL),

	BT_GATT_CHARACTERISTIC(BLE_MAS_TX_UUID, BT_GATT_CHRC_NOTIFY,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL, NULL),

	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
};

/* thread to receive data from uart device */
static void char_thread_entry(void *p1, void *p2, void *p3) {
	int ret;
	uint8_t *recv_buffer = NULL;

	if (bt_mgnt && bt_mgnt->sppble_stream) {
		ret = stream_open(bt_mgnt->sppble_stream, MODE_IN_OUT);
		if (ret) {
			printk("stream_open Failed\n");
		} else {
			bt_mgnt->sppble_stream_opened = 1;
		}
	}

	recv_buffer = (uint8_t *)malloc(MAS_BT_SEND_LEN_ONCE);
	memset(recv_buffer,0,MAS_BT_SEND_LEN_ONCE);
	while (1) {
		if (bt_mgnt && bt_mgnt->sppble_stream && bt_mgnt->sppble_stream_opened) {
			ret = stream_read(bt_mgnt->sppble_stream, recv_buffer, MAS_BT_SEND_LEN_ONCE);
			if (ret > 0) {
				//printk("sppble_stream rx: %d\n", ret);
				mas_link_char_upload_recv_data(bt_mgnt->g_link_char, recv_buffer, ret);
			} else {
				printk("char_thread_entry exit.\n");
				break;
			}
		}
	}
	
	stream_close(bt_mgnt->sppble_stream);
	bt_mgnt->sppble_stream_opened = 0;
	free(recv_buffer);
}

static void mas_connect_cb(int connected, uint8_t connect_type)
{
	//uint16_t mtu;

	printk("connect: %d connect_type %d.\n", connected, connect_type);

	/* avoid connect again after exit */
	if (bt_mgnt) {

		if (!connected) {
			bt_mgnt->connect_type = NONE_CONNECT_TYPE;
			mas_low_level_post_status_changed_event(MAS_STATUS_SERVER_LINK_DOWN);
		} else {
			k_thread_create(&bt_mgnt->mas_rx_thread, mas_rx_stack, 
				K_THREAD_STACK_SIZEOF(mas_rx_stack), char_thread_entry, NULL, NULL, NULL, -3, 0, K_NO_WAIT);
			k_thread_name_set(&bt_mgnt->mas_rx_thread, "mas_read");
			k_thread_start(&bt_mgnt->mas_rx_thread);

			bt_mgnt->connect_type = connect_type;
			mas_low_level_post_status_changed_event(MAS_STATUS_SERVER_LINK_UP);
		}
	}
}

static struct sppble_stream_init_param bt_init_param = {
	.spp_uuid = (uint8_t *)mas_spp_uuid,
	.gatt_attr = mas_attrs,
	.attr_size = ARRAY_SIZE(mas_attrs),
	.tx_chrc_attr = &mas_attrs[3],
	.tx_attr = &mas_attrs[4],
	.tx_ccc_attr = &mas_attrs[5],
	.rx_attr = &mas_attrs[2],
	.connect_cb = (void *)mas_connect_cb,
	.read_timeout = OS_FOREVER,
	.write_timeout = OS_FOREVER,
	.read_buf_size = 2048,
};

static size_t send_blue_data(struct mas_link_char *link_char, const void *buffer, size_t size) {
	int ret = 0;
	size_t send_len, res_len, offset;
	uint8_t *ptr = (uint8_t *)buffer;
	printk("send size: %d\n", size);

	offset = 0;
	res_len = size;
	do {
		if (res_len > MAS_BT_SEND_LEN_ONCE) {
			send_len = MAS_BT_SEND_LEN_ONCE;
		} else {
			send_len = res_len;
		}

		if (bt_mgnt && bt_mgnt->sppble_stream) {
			ret = stream_write(bt_mgnt->sppble_stream, &ptr[offset], send_len);
			if (ret > 0) {
				//printk("sppble_stream tx: %d\n", ret);
			}
		}

		offset += send_len;
		res_len -= send_len;

	} while (res_len > 0);

	return offset;
}

static const mas_link_char_port_ops ops = {send_blue_data};

extern "C" int bluetooth_register(void)
{
	if (bt_mgnt) {
		printk("bt init already!\n");
		return -1;
	}

	bt_mgnt = (struct rt_bluetooth_mgnt_device *)malloc(sizeof(struct rt_bluetooth_mgnt_device));
	if (!bt_mgnt) {
		printk("bt_mgnt malloc fail!\n");
		return -1;
	}

	memset(bt_mgnt ,0, sizeof(struct rt_bluetooth_mgnt_device));
	bt_mgnt->sppble_stream = sppble_stream_create((void *)&bt_init_param);
	if (!bt_mgnt->sppble_stream) {
		printk("stream_create failed\n");
		free(bt_mgnt);
		return -1;
	}

	bt_mgnt->g_link_char = (struct mas_link_char *)malloc(sizeof(struct mas_link_char));
	memset(bt_mgnt->g_link_char, 0, sizeof(struct mas_link_char));
	return mas_link_bluetooth_register(bt_mgnt->g_link_char, &ops);
}

