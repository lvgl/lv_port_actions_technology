/*
 * Copyright (c) 2019 Actions Semiconductor Co, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nsm_app_inner.h"
#include "nsm_test_inner.h"

static void nsm_test_start(void)
{
    nsm_init();
}

static void nsm_test_stop(void)
{
    SYS_LOG_INF("ok");
    nsm_deinit();
}

void nsm_test_backend_callback(int cmd, int state)
{
    // int err;

    SYS_LOG_INF("spp test call back cmd %d state %d", cmd, state);
    if (cmd == SPP_TEST_BACKEND_START_STATE) {
        if (state == 1)
        {
            nsm_test_start();
        }
        else 
        {
            nsm_test_stop();
        }
    }
}

/* "0000E005-0000-1000-8000-00805F9B34FB"  spp test uuid */
//static const u8_t test_spp_uuid[16] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x05, 0xe0, 0x00, 0x00};

#if 1

/* BLE */
#define BLE_NSM_SERVICE_UUID           BT_UUID_DECLARE_16(0xAA01)
#define BLE_NSM_RX_UUID                BT_UUID_DECLARE_16(0xAA02)
#define BLE_NSM_TX_UUID                BT_UUID_DECLARE_16(0xAA03)

static struct bt_gatt_attr nsm_gatt_attr[] = {
    BT_GATT_PRIMARY_SERVICE(BLE_NSM_SERVICE_UUID),

    BT_GATT_CHARACTERISTIC(BLE_NSM_RX_UUID, BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                        BT_GATT_PERM_WRITE, NULL, NULL, NULL),

    BT_GATT_CHARACTERISTIC(BLE_NSM_TX_UUID, BT_GATT_CHRC_NOTIFY,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, NULL, NULL, NULL),

    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
};
#endif

static const struct ota_backend_bt_init_param bt_init_param = {
    .spp_uuid = NULL,
    .gatt_attr = &nsm_gatt_attr[0],
    .attr_size = ARRAY_SIZE(nsm_gatt_attr),
    .tx_chrc_attr = &nsm_gatt_attr[3],
    .tx_attr = &nsm_gatt_attr[4],
    .tx_ccc_attr = &nsm_gatt_attr[5],
    .rx_attr = &nsm_gatt_attr[2],
    .read_timeout = OS_FOREVER, /* K_FOREVER, K_NO_WAIT,  K_MSEC(ms) */
    .write_timeout = OS_FOREVER,
};

int nsm_test_app_init(void)
{
    SYS_LOG_INF("nsm_test_app_init");

    if (!nsm_test_backend_load_bt_init(nsm_test_backend_callback, (struct ota_backend_bt_init_param *)&bt_init_param))
    {
        SYS_LOG_INF("failed");
        return -ENODEV;
    }

    return 0;
}

void nsm_spp_test_main(void *p1, void *p2, void *p3)
{

    SYS_LOG_INF("Enter");

    while (!nsm_is_quitting())
    {
        if (!nsm_cmd_xml_parse())
        {
            SYS_LOG_INF("no data.");
            os_sleep(1);
        }
    }

#if 0
    do
    {
        // ret = ASET_Protocol_Rx_Fsm(&g_tool_data.usp_handle, asqt_data->data_buffer, sizeof(asqt_data->data_buffer));
        ret = spp_test_backend_read(&rx_data[wr], 1, 5000);
        if (ret)
        {
            wr++;
            if (wr > 15)
            {
                SYS_LOG_ERR("wr %d.",wr);

                for (i = 0;i < 16; i++)
                {
                    printf("%x",rx_data[i]);
                }
                printf("\n");
                wr = 0;
            }

            if (wr == nsm_cmd_xml(rx_data, wr))
         }
         else
         {
             SYS_LOG_INF("no data.");
             os_sleep(1);
         }
    } while (!g_nsm_data.quited);
#endif
    g_nsm_data.quited = 1;
}

