/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tool_app_inner.h"
#include "tool_att.h"
#include <sys_wakelock.h>
#include <srv_manager.h>
#include <property_manager.h>

#ifdef CONFIG_MEDIA
#include <media_player.h>
#endif

#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif
#include <soc.h>

u8_t __aligned(ARCH_STACK_PTR_ALIGN) Z_GENERIC_SECTION(.attruntimedata) att_runtime_data[0x4000];

static int (*att_code_entry)(void *p1, void *p2, void *p3);

int act_open_test_file(const char *file_name)
{
    return stub_write_packet(STUB_CMD_ATT_FOPEN, 0, (u8_t*)file_name, strlen(file_name));
}

int act_read_test_file(u8_t *data, u32_t offset, u32_t bytes)
{
	int ret_val;
    ret_val = stub_read_packet(STUB_CMD_ATT_FREAD, offset, data, bytes);
	return ret_val;
}

int act_test_start_deal(void)
{
    //start ATT test
    return stub_write_packet(STUB_CMD_ATT_START, STRING_CODING_UTF_8, NULL, 0);
}

/**
 *  @brief  read data from atf file
 *
 *  @param  dst_addr  : buffer address
 *  @param  dst_buffer_len : buffer size
 *  @param  offset    : offset of the atf file head
 *  @param  total_len : read bytes
 *
 *  @return  negative errno code on fail, or return read bytes
 */
int read_atf_file_by_stub(u8_t *dst_addr, u32_t dst_buffer_len, u32_t offset, s32_t total_len)
{
#define MAX_READ_LEN 1024
    int ret_val = 0;

    u32_t cur_file_offset = offset;
    s32_t left_len = total_len;

    SYS_LOG_INF("data_info -> offset:%d total_len:%d\n", offset, total_len);
    while (left_len > 0) {
        u32_t read_len = left_len;
        if (read_len >= MAX_READ_LEN)
            read_len = MAX_READ_LEN;
		SYS_LOG_INF("left_len %d",left_len);
        //The size of the actual read load data must be aligned with four bytes, otherwise the read fails
        u32_t aligned_len = (read_len + 3) / 4 * 4;

        //Send read command
        ret_val = act_read_test_file(dst_addr, cur_file_offset, aligned_len);
        if (ret_val <= 0) {
            SYS_LOG_INF("att_readfile_data ret:%d\n", ret_val);
            ret_val = -1;
			break;
        }

        dst_addr += read_len;
        left_len -= read_len;
        cur_file_offset += read_len;
    }

    if (ret_val == 0) {
		SYS_LOG_INF("read:%d bytes ok!\n", total_len);
	}

    return (ret_val < 0) ? ret_val : total_len;
}

/**
 *  @brief  read data from atf file
 *
 *  @param  dst_addr  : buffer address
 *  @param  dst_buffer_len : buffer size
 *  @param  offset    : offset of the atf file head
 *  @param  read_len  : read bytes
 *
 *  @return  negative errno code on fail, or return read bytes
 */
int read_atf_file(u8_t *dst_addr, u32_t dst_buffer_len, u32_t offset, s32_t read_len)
{
    s32_t ret = -1;

    ret = read_atf_file_by_stub(dst_addr, dst_buffer_len, offset, read_len);

    return ret;
}

/**
 *  @brief  read the attr of one sub file in atf file by sub file name
 *
 *  @param  sub_file_name : sub file name
 *  @param  sub_atf_dir   : return parameter, the attr of the sub file
 *
 *  @return  0 on success, negative errno code on fail.
 */
int atf_read_sub_file_attr(const u8_t *sub_file_name, atf_dir_t *sub_atf_dir)
{
	int ret_val;
	int i, j, cur_files;
	int read_len;
	int atf_offset = 32;
	atf_dir_t *cur_atf_dir;
	atf_dir_t *atf_dir_buffer;

    if (NULL == sub_file_name || NULL == sub_atf_dir) {
    	return -1;
    }

	atf_dir_buffer = (atf_dir_t *) app_mem_malloc(512);
	if (NULL == atf_dir_buffer) {
		return -1;
	}

	i = 32;
	while (i > 0) {
		if (i >= 16) {
			cur_files = 16;
			read_len = 512;
		} else {
			cur_files = i;
			read_len = i*32;
		}
		ret_val = read_atf_file((u8_t *) atf_dir_buffer, 512, atf_offset, read_len);
	    if (ret_val < read_len) {
	    	SYS_LOG_ERR("read_atf_file fail\n");
	    	break;
	    }

		cur_atf_dir = atf_dir_buffer;
		for (j = 0; j < cur_files; j++)
	    {
	        if (strncmp(sub_file_name, cur_atf_dir->filename, ATF_MAX_SUB_FILENAME_LENGTH) == 0)
	        {
	            memcpy(sub_atf_dir, cur_atf_dir, sizeof(atf_dir_t));
	            ret_val = 0;
	            goto read_exit;
	        }

	        cur_atf_dir++;
	    }

		atf_offset += read_len;
		i -= cur_files;
	}

	ret_val = -1;
    SYS_LOG_INF("can't find %s\n", sub_file_name);

read_exit:
	app_mem_free(atf_dir_buffer);
    return ret_val;
}

/**
 *  @brief  read data from one sub file in atf file by name
 *
 *  @param  dst_addr  : buffer address
 *  @param  dst_buffer_len : buffer size
 *  @param  sub_file_name : sub file name
 *  @param  offset    : offset of the atf file head
 *  @param  read_len  : read bytes
 *
 *  @return  negative errno code on fail, or return read bytes
 */
int read_atf_sub_file(u8_t *dst_addr, u32_t dst_buffer_len, const u8_t *sub_file_name, s32_t offset, s32_t read_len, atf_dir_t *sub_atf_dir)
{
    int ret = atf_read_sub_file_attr(sub_file_name, sub_atf_dir);
    if (ret != 0)
        return -1;

	SYS_LOG_INF("sub_atf_dir offset = 0x%x, len = %x, load = %x, run = %x\n",
			sub_atf_dir->offset, sub_atf_dir->length, sub_atf_dir->load_addr, sub_atf_dir->run_addr);

    if (read_len < 0)
    {
        read_len = sub_atf_dir->length - offset;
    }

    if (read_len < 0)
    {
        SYS_LOG_INF("read_len <= 0\n");
        return -1;
    }

    if (read_len > dst_buffer_len)
    {
        SYS_LOG_INF("read_len:%d > dst_buffer_len:%d\n", read_len, dst_buffer_len);
        return -1;
    }

    if (offset + read_len > sub_atf_dir->length)
    {
        SYS_LOG_INF("sub_file_name%s, offset_from_sub_file:%d + read_len:%d > sub_file_length:%d\n",
                sub_file_name, offset, read_len, sub_atf_dir->length);
        return -1;
    }

    dst_addr = (u8_t *) sub_atf_dir->load_addr;

    return read_atf_file(dst_addr, dst_buffer_len, sub_atf_dir->offset + offset, read_len);
}


void att_test_exit(void)
{
    SYS_LOG_INF("test ap end...");

    while(1)
    {
        os_sleep(100);
    }
}




void autotest_start(void)
{
	int ret_val;

	atf_dir_t sub_atf_dir;

	act_test_start_deal();

	ret_val = act_open_test_file("atttest.bin");
	if (ret_val != 0) {
		SYS_LOG_ERR("read file failed\n");
		return;
	}

	ret_val = read_atf_sub_file(NULL, 0x1000, "loader.bin", 0, -1, &sub_atf_dir);
	if (ret_val <= 0) {
		SYS_LOG_ERR("read pattern file failed\n");
		return;
	}

	att_code_entry = (int (*)(void *, void *, void *))((void *)(sub_atf_dir.run_addr));
	if(!att_code_entry){
		return;
	}

	SYS_LOG_INF("load entry %p\n", att_code_entry);

	app_manager_active_app(APP_ID_ATT);
}

static int _stdout_hook_null(int c)
{
	(void)(c);

	return EOF;
}

void att_test_enter(usp_handle_t *usp_handle)
{
    bool pass = false;

	// print again, in case string is lose in ATT tools
	//printf("dbg tool att\n");

	extern void __printk_hook_install(int (*fn)(int));
	extern void __stdout_hook_install(int (*fn)(int));
	__printk_hook_install(_stdout_hook_null);
	__stdout_hook_install(_stdout_hook_null);

    // success
    // led_ctrl_set_state(LED_RED,  LED_STATE_ON);
    // led_ctrl_set_state(LED_BLUE, LED_STATE_OFF);

    InitStub(usp_handle);
	stub_max_payload_modify(1024, 1024);

    //if ((0 == OpenStub(500)) && (0 == test_dispatch()))
    if ((0 == OpenStub(5000)))
    {
    	SYS_LOG_INF("%d", __LINE__);
    	autotest_start();
        pass = true;
    }

    SYS_LOG_INF("exit");
#if 0
    CloseStub();
    ExitStub(usp_handle);

    if (!pass)
    {
        // fail
        // led_ctrl_set_state(LED_RED,  LED_STATE_OFF);
        // led_ctrl_set_state(LED_BLUE, LED_STATE_ON);
    }
#endif
}



void tool_att_loop(void *p1, void *p2, void *p3)
{
    SYS_LOG_INF("Enter");

#ifdef CONFIG_MEDIA
	media_player_force_stop(true);
#endif

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
#endif

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
    dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "tool");
#endif

    tool_uart_init(4096);
    att_test_enter(&g_tool_data.usp_handle);

#if 0
    att_test_exit();

    tool_uart_exit();
    g_tool_data.quited = 1;

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
    dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "tool");
#endif

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock(FULL_WAKE_LOCK);
#endif

    SYS_LOG_INF("Exit");
#endif
}

static void att_sleep(s32_t sleep_ms)
{
	k_msleep(sleep_ms);
}
static void * att_tool_malloc(size_t size)
{
	return app_mem_malloc(size);
}


static const att_interface_api_t att_api =
{
    .version = 0x01,
	.compiler_type = 1,
	.property_get = property_get,
	.property_set = property_set,
	.property_set_factory = property_set_factory,

	.stub_write_packet = stub_write_packet,
	.stub_read_packet = stub_read_packet,
	.stub_status_inquiry = stub_status_inquiry,

	.read_atf_sub_file = read_atf_sub_file,
	.malloc = att_tool_malloc,
	.free = app_mem_free,
	.k_sleep = att_sleep,
	.k_uptime_get = (void *)k_uptime_get,
	.udelay = z_impl_k_busy_wait,
	.vprintk = vprintk,
};

static void autotest_main_start(void *p1, void *p2, void *p3)
{
	if(att_code_entry){
		att_code_entry(p1, p2, p3);
	}
}

/*
att At the end of the test, it is decided whether to shut down, restart, or boot directly
*/
void test_exit(void)
{
#ifdef CONFIG_PROPERTY
	property_flush(NULL);
#endif

#ifdef CONFIG_ATT_CYCLIC_TEST
	att_sleep(3000);
    system_power_reboot(REBOOT_TYPE_NORMAL);
#else
	sys_pm_poweroff();
#endif
}



APP_DEFINE(att, share_stack_area, sizeof(share_stack_area), CONFIG_APP_PRIORITY,
	   FOREGROUND_APP, (void *)&att_api, (void *)test_exit, NULL,
	   autotest_main_start, NULL);

