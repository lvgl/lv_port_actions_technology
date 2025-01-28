/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file esd manager interface
 */

#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "esd_manager"

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>

#include <string.h>

#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif

#ifdef CONFIG_PLAYTTS
#include "tts_manager.h"
#endif

#ifdef CONFIG_THREAD_TIMER
#include "thread_timer.h"
#endif

#define ESD_BK_FLAG  0x12345678
#define BK_INFO_SIZE 250
#define BK_INFO_FINISED_FLAG 0xAA

OS_MUTEX_DEFINE(esd_manager_mutex);

static struct thread_timer esd_reset_timer;

typedef struct {
	uint32_t flag;
	uint8_t  reset_flag;
	uint8_t  bk_off;
	uint8_t  bk_info[BK_INFO_SIZE];
} esd_bk_info_t;/* max length is 0x100 bytes */

static __in_section_unique(ESD_DATA) esd_bk_info_t gloable_esd_bk_info;

static void _esd_manager_reset_timer(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	os_mutex_lock(&esd_manager_mutex, OS_FOREVER);

	if (esd_manager_check_esd()) {
	#ifdef CONFIG_PLAYTTS
		tts_manager_unlock();
	#endif
		esd_manager_reset_finished();
	}

	os_mutex_unlock(&esd_manager_mutex);
}

void esd_manager_init(void)
{
	esd_bk_info_t *esd_bk_info  = &gloable_esd_bk_info;

	/* print_buffer(&gloable_esd_bk_info, 1, sizeof(esd_bk_info_t), 16, 0); */

	if (esd_bk_info->flag == ESD_BK_FLAG) {
		esd_bk_info->reset_flag = 1;
	} else {
		esd_bk_info->reset_flag = 0;
		esd_bk_info->bk_off = 0;
		memset(&gloable_esd_bk_info, 0, sizeof(esd_bk_info_t));
	}

	thread_timer_init(&esd_reset_timer, _esd_manager_reset_timer, NULL);

	thread_timer_start(&esd_reset_timer, 20000, 0);

	SYS_LOG_INF("0x%x flag %d\n", esd_bk_info->flag, esd_bk_info->reset_flag);
}

bool esd_manager_check_esd(void)
{
	esd_bk_info_t *esd_bk_info  = &gloable_esd_bk_info;

	return (esd_bk_info->reset_flag == 1);
}

void esd_manager_reset_finished(void)
{
	esd_bk_info_t *esd_bk_info  = &gloable_esd_bk_info;

	esd_bk_info->reset_flag = 0;

	SYS_LOG_INF("esd finished\n");
}

int esd_manager_save_scene(int tag, uint8_t *value, int len)
{
	int i = 0;
	int tmp_tag, tmp_len = 0;
	esd_bk_info_t *esd_bk_info  = &gloable_esd_bk_info;

	if (esd_bk_info->bk_off + len + 2 > BK_INFO_SIZE) {
		return -ENOMEM;
	}

	os_mutex_lock(&esd_manager_mutex, OS_FOREVER);

	for (i = 0; i < BK_INFO_SIZE - 2;) {
		tmp_tag = esd_bk_info->bk_info[i];
		tmp_len = esd_bk_info->bk_info[i + 1];
		if (tmp_tag == tag && tmp_len >= len) {
			memcpy(&esd_bk_info->bk_info[i + 2], value, len);
			goto exit;
		}

		if (tmp_tag == BK_INFO_FINISED_FLAG) {
			break;
		}
		i += tmp_len + 2;
	}

	esd_bk_info->bk_info[esd_bk_info->bk_off] = tag;
	esd_bk_info->bk_info[esd_bk_info->bk_off + 1] = len;

	memcpy(&esd_bk_info->bk_info[esd_bk_info->bk_off + 2], value, len);

	esd_bk_info->bk_off += len + 2;
	esd_bk_info->bk_info[esd_bk_info->bk_off] = BK_INFO_FINISED_FLAG;

	esd_bk_info->flag = ESD_BK_FLAG;

	/* print_buffer(&gloable_esd_bk_info, 1, sizeof(esd_bk_info_t), 16, 0); */
	/* SYS_LOG_INF("tag %d value %x len %d\n", tag, value[0], len); */
exit:
	os_mutex_unlock(&esd_manager_mutex);
	return 0;
}

int esd_manager_restore_scene(int tag, uint8_t *value, int len)
{
	int result = -1;
	int i = 0;
	int tmp_tag, tmp_len = 0;
	esd_bk_info_t *esd_bk_info  = &gloable_esd_bk_info;

	os_mutex_lock(&esd_manager_mutex, OS_FOREVER);
	for (i = 0; i < BK_INFO_SIZE - 2;) {
		tmp_tag = esd_bk_info->bk_info[i];
		tmp_len = esd_bk_info->bk_info[i + 1];
		if (tmp_tag == tag && tmp_len <= len) {
			memcpy(value, &esd_bk_info->bk_info[i + 2], tmp_len);
			result = 0;
			break;
		}

		if (tmp_tag == BK_INFO_FINISED_FLAG) {
			break;
		}

		i += tmp_len + 2;
	}
	/* SYS_LOG_INF("tag %d value %x len %d\n", tag, value[0], len); */
	/* print_buffer(&gloable_esd_bk_info, 1, sizeof(esd_bk_info_t), 16, 0); */
	os_mutex_unlock(&esd_manager_mutex);
	return result;
}

void esd_manager_deinit(void)
{
#if CONFIG_SYS_LOG_DEFAULT_LEVEL  > 2
	esd_bk_info_t *esd_bk_info  = &gloable_esd_bk_info;
#endif
	memset(&gloable_esd_bk_info, 0, sizeof(esd_bk_info_t));
#if CONFIG_SYS_LOG_DEFAULT_LEVEL  > 2
	SYS_LOG_INF("0x%x flag %d\n", esd_bk_info->flag, esd_bk_info->reset_flag);
#endif
}



