/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include "tool_app_inner.h"
#include "tts_manager.h"
#include <drivers/uart.h>
#include <spp_test_backend.h>
#include <soc_pm.h>
#include <msg_manager.h>

act_tool_data_t g_tool_data;

#ifdef CONFIG_ATT_SUPPORT_MULTIPLE_DEVICES
/* ascending sequence, and the bigest value should be 880(0x370) or lower */
static const u16_t tool_adc2serial[] = {0, 90, 180, 270, 360, 450, 540, 630, 720, 880};
#endif

void shell_dbg_disable(void);

int tool_init(char *type, unsigned int dev_type)
{
    void (*tool_process_deal)(void *p1, void *p2, void *p3) = NULL;

	// if (g_tool_data.stack)
		// return -EALREADY;

    SYS_LOG_INF("stack %p %s\n", g_tool_data.stack, type);

    if (NULL == g_tool_data.stack)
    {
	    g_tool_data.stack = app_mem_malloc(CONFIG_TOOL_STACK_SIZE);
    }
	else
	{
		//if tools thread not exit, cannot init again
		if(!g_tool_data.quited){
			return -EALREADY;
		}
	}

	if (!g_tool_data.stack)
    {
		SYS_LOG_ERR("tool stack mem_malloc failed");
		return -ENOMEM;
	}

    memset(&g_tool_data.usp_handle, 0, sizeof(usp_handle_t));

    if (TOOL_DEV_TYPE_SPP == dev_type)
    {
#ifdef CONFIG_SPP_TEST_SUPPORT
        // g_tool_data.dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
        // data's correctness is guaranteed by USP protocol
        g_tool_data.usp_handle.usp_protocol_global_data.transparent_bak = 1;
        // API initial
        g_tool_data.usp_handle.api.read    = (void*)spp_test_backend_read;
        g_tool_data.usp_handle.api.write   = (void*)spp_test_backend_write;
        g_tool_data.usp_handle.api.ioctl   = (void*)spp_test_backend_ioctl;
        tool_process_deal = tool_spp_test_main;
#endif
    }
    else
    {
        g_tool_data.dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
        // data's correctness is guaranteed by USP protocol
        // g_tool_data.usp_handle.usp_protocol_global_data.transparent_bak = 0;
        // API initial
        g_tool_data.usp_handle.api.read    = (void*)tool_uart_read;
        g_tool_data.usp_handle.api.write   = (void*)tool_uart_write;
        g_tool_data.usp_handle.api.ioctl   = (void*)tool_uart_ioctl;

        if(0 == strcmp(type, "aset"))
        {
            g_tool_data.type = USP_PROTOCOL_TYPE_ASET;
            tool_process_deal = tool_aset_loop;
        }
#ifdef CONFIG_ACTIONS_ATT
        else if(0 == strcmp(type, "att"))
        {
            g_tool_data.type = USP_PROTOCOL_TYPE_STUB;
            tool_process_deal = tool_att_loop;
        }
#endif
        else
        {
            return -1;
        }
    }

	g_tool_data.dev_type = dev_type;
	g_tool_data.quit = 0;
	g_tool_data.quited = 0;

	// os_sem_init(&g_tool_data.init_sem, 0, 1);
	os_thread_create(g_tool_data.stack, CONFIG_TOOL_STACK_SIZE,
			tool_process_deal, NULL, NULL, NULL, 8, 0, 0);

	// os_sem_take(&g_tool_data.init_sem, OS_FOREVER);

	SYS_LOG_INF("begin trying to connect pc tool");
	return g_tool_data.type;
}

void tool_deinit(void)
{
	if (!g_tool_data.stack)
		return;

	SYS_LOG_WRN("will disconnect pc tool, then exit");

	g_tool_data.quit = 1;
	while (!g_tool_data.quited)
		os_sleep(1);

	app_mem_free(g_tool_data.stack);
	g_tool_data.stack = NULL;

	SYS_LOG_INF("exit now");
}

static void tool_init_sysrq(const struct device *port)
{
	struct app_msg msg;

	memset(&msg, 0, sizeof(msg));

	msg.type = MSG_APP_TOOL_INIT;

	send_async_msg(APP_ID_MAIN, &msg);
}

void tool_sysrq_init(void)
{
	tool_init(g_tool_data.type_buf, TOOL_DEV_TYPE_UART);
}

static void tool_init_work(struct k_work *work)
{
	const struct device *dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);

	if(dev){
		tool_init_sysrq(dev);
	}
}


/* magic sysrq key: CTLR + 'A' 'C' 'T' 'I' 'O' 'N' 'S' */
static const char sysrq_toolbuf[] = {0x01, 0x03, 0x20, 0x09, 0x15, 0x14, 0x19};

static uint8_t sysrq_tool_idx;

extern void printk_dma_switch(int sw_dma);

static const uint8_t tool_ack[] = {0x05, 0x00, 0x00, 0x00, 0x00, 0xcc};

void handle_sysrq_tool_main(const struct device * port, int key_size)
{
	char key;
	char key_buf[32];
	int idx, i, j, key_index;

	//must called by application init, avoid printk switch dma crash
	if (!g_tool_data.init_printk_hw){
		return;
	}

	//only call once
	if (g_tool_data.init_work){
		return;
	}

	idx = 0;
	i = 0;
	j = 0;
	key_index = 0;

#ifdef CONFIG_SHELL_DBG
	shell_dbg_disable();
#endif

#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(0);
#endif

	//check receive tools type
	while(1){
		uart_poll_in(port, &key);

		if ((idx < 32) && (idx >= 0)) {
			key_buf[idx++] = key;
		}

		if(key != 0x0a && idx < 32){
			continue;
		}

		j = 0;
		for(i = 0; i < idx; i++){
			if ((i < 32) && (i >= 0)) {
				if(key_buf[i] == sysrq_toolbuf[j]){
					if(j == 0){
						key_index = i;
					}
					j++;

					if(j == key_size){
						break;
					}
				}
			}
		}

		if(j == key_size){
			break;
		}

		idx = 0;
	}

	i++;

	for(j = i; j < idx; j++){
		if ((j < 32) && (j >= 0)) {
			if(key_buf[j] == 0x0a){
				key_buf[j] = 0;
				break;
			}
		}
	}

#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(1);
#endif

    if (i > 32) {
        SYS_LOG_ERR("parse tool err:%d\n", i);
        return;
    }

	SYS_LOG_INF("parse tool type:%s\n", &key_buf[i]);

	//write ack to tools
	//printk("%s\n", &key_buf[key_index]);
	tool_uart_write(tool_ack, sizeof(tool_ack), 0);

	memcpy(g_tool_data.type_buf, &key_buf[i], j - i);
    if (0 == strcmp(g_tool_data.type_buf, "reboot"))
    {
        SYS_LOG_INF("reboot system");
        k_busy_wait(200 * 1000);
        sys_pm_reboot(0);
    }

	g_tool_data.init_work = true;

	k_work_init(&g_tool_data.work, tool_init_work);

	k_work_submit(&g_tool_data.work);
}

static void sysrq_tool_handler(const struct device * port, char c)
{
	if (c == sysrq_toolbuf[sysrq_tool_idx]) {
		sysrq_tool_idx++;
		if (sysrq_tool_idx == sizeof(sysrq_toolbuf)) {
			sysrq_tool_idx = 0;
			handle_sysrq_tool_main(port, sizeof(sysrq_toolbuf));
		}
	} else {
		sysrq_tool_idx = 0;
	}
}

extern void sysrq_register_user_handler(void (*callback)(const struct device * port, char c));

static int uart_data_init(const struct device *dev)
{
	memset(&g_tool_data, 0, sizeof(g_tool_data));

	sysrq_register_user_handler(sysrq_tool_handler);

	g_tool_data.init_printk_hw = true;

	return 0;
}

SYS_INIT(uart_data_init, APPLICATION, 10);