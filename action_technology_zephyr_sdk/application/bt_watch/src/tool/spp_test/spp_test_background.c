/*
 * Copyright (c) 2019 Actions Semiconductor Co, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spp_test_inner.h"
#include "abtp_inner.h"
#include "mic_test_inner.h"

static void spp_test_start(void)
{
    tool_init("SPP", TOOL_DEV_TYPE_SPP);
}

static void spp_test_stop(void)
{
	SYS_LOG_INF("ok");
    tool_deinit();
}

void spp_test_backend_callback(int cmd, int state)
{
	// int err;

	SYS_LOG_INF("spp test call back cmd %d state %d", cmd, state);
	if (cmd == SPP_TEST_BACKEND_START_STATE) {
		if (state == 1)
        {
			spp_test_start();
		}
        else 
        {
			spp_test_stop();
		}
	}
}

/* UUID order using BLE format */
/*static const u8_t ota_spp_uuid[16] = {0x00,0x00,0x66,0x66, \
	0x00,0x00,0x10,0x00,0x80,0x00,0x00,0x80,0x5F,0x9B,0x34,0xFB};*/

/* "0000E004-0000-1000-8000-00805F9B34FB"  spp test uuid */
static const u8_t test_spp_uuid[16] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, \
	0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x04, 0xe0, 0x00, 0x00};

#if 0
/* BLE */
/*	"e49a25f8-f69a-11e8-8eb2-f2801f1b9fd1" reverse order  */
#define OTA_SERVICE_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xf8, 0x25, 0x9a, 0xe4)

/* "e49a25e0-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define OTA_CHA_RX_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xe0, 0x25, 0x9a, 0xe4)

/* "e49a28e1-f69a-11e8-8eb2-f2801f1b9fd1" reverse order */
#define OTA_CHA_TX_UUID BT_UUID_DECLARE_128( \
				0xd1, 0x9f, 0x1b, 0x1f, 0x80, 0xf2, 0xb2, 0x8e, \
				0xe8, 0x11, 0x9a, 0xf6, 0xe1, 0x28, 0x9a, 0xe4)

static struct bt_gatt_attr ota_gatt_attr[] = {
	BT_GATT_PRIMARY_SERVICE(OTA_SERVICE_UUID),

	BT_GATT_CHARACTERISTIC(OTA_CHA_RX_UUID, BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
						BT_GATT_PERM_WRITE, NULL, NULL, NULL),

	BT_GATT_CHARACTERISTIC(OTA_CHA_TX_UUID, BT_GATT_CHRC_NOTIFY,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL, NULL),

	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
};
#endif

static const struct ota_backend_bt_init_param bt_init_param = {
	.spp_uuid = test_spp_uuid,
	.gatt_attr = NULL,
	.attr_size = 0,
	.tx_chrc_attr = NULL,
	.tx_attr = NULL,
	.tx_ccc_attr = NULL,
	.rx_attr = NULL,
	.read_timeout = OS_NO_WAIT,	/* OS_FOREVER, OS_NO_WAIT,  OS_MSEC(ms) */
	.write_timeout = OS_NO_WAIT,
};

int spp_peer_long_data_cb(char* param, uint32_t param_len)
{
	SYS_LOG_INF("length %d.", param_len);
	return 0;
}

int spp_test_app_init(void)
{
	SYS_LOG_INF("");

	if (!spp_test_backend_load_bt_init(spp_test_backend_callback, (struct ota_backend_bt_init_param *)&bt_init_param))
    {
		SYS_LOG_INF("failed");
		return -ENODEV;
	}

	return 0;
}

void tool_spp_test_main(void *p1, void *p2, void *p3)
{
    u8_t head_data;
    int ret;

    ret = spp_test_backend_read(&head_data, 1, 5000);
    if (1 != ret)
    {
        SYS_LOG_ERR("NO SPP data RCV");
        g_tool_data.quited = 1;
        return;
    }

    SYS_LOG_INF("%x", head_data);
	if (USP_PACKET_TYPE_DATA == head_data)
    {
        // discard first ASET connect packet
        spp_test_backend_read(NULL, 5, 0);
        tool_aset_loop(NULL, NULL, NULL);
    }
    else if ('A' == head_data)
    {
        // recieve string "ACTIONS"
        spp_test_backend_read(NULL, 6, 0);
        show_version_through_spp();
    }
    else if (ABTP_HEAD_MAGIC == head_data)
    {
        abtp_communicate(head_data);
    }
    else if (MIC_TEST_HEAD_MAGIC == head_data)
    {
        mic_test_communicate(head_data);
    }

    g_tool_data.quited = 1;
}

