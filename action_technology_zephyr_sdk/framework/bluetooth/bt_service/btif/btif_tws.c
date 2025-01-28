/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt srv tws api interface
 */

#define SYS_LOG_DOMAIN "btif_tws"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"

int btif_tws_register_processer(void)
{
#ifdef CONFIG_SUPPORT_TWS
	return btsrv_register_msg_processer(MSG_BTSRV_TWS, &btsrv_tws_process);
#else
	return 0;
#endif
}

int btif_tws_init(btsrv_tws_callback cb)
{
	return btsrv_function_call(MSG_BTSRV_TWS, MSG_BTSRV_TWS_INIT, cb);
}

int btif_tws_wait_pair(int try_times)
{
	return btsrv_function_call(MSG_BTSRV_TWS, MSG_BTSRV_TWS_WAIT_PAIR, (void *)try_times);
}

int btif_tws_cancel_wait_pair(void)
{
	return btsrv_function_call(MSG_BTSRV_TWS, MSG_BTSRV_TWS_CANCEL_WAIT_PAIR, NULL);
}

int btif_tws_disconnect(bd_address_t *addr)
{
	return btsrv_function_call_malloc(MSG_BTSRV_TWS, MSG_BTSRV_TWS_DISCONNECT, (void *)addr, sizeof(bd_address_t), 0);
}

int btif_tws_can_do_pair(void)
{
#ifdef CONFIG_SUPPORT_TWS
	int ret, flags;

	flags = btsrv_set_negative_prio();
	ret = btsrv_tws_can_do_pair();
	btsrv_revert_prio(flags);

	return ret;
#else
	return 0;
#endif
}

int btif_tws_is_in_connecting(void)
{
#ifdef CONFIG_SUPPORT_TWS
	int ret, flags;

	flags = btsrv_set_negative_prio();
	ret = btsrv_tws_is_in_connecting();
	btsrv_revert_prio(flags);

	return ret;
#else
	return 0;
#endif
}

int btif_tws_get_dev_role(void)
{
	int ret, flags;

	flags = btsrv_set_negative_prio();
	ret = btsrv_rdm_get_dev_role();
	btsrv_revert_prio(flags);

	return ret;
}

int btif_tws_set_input_stream(io_stream_t stream)
{
#ifdef CONFIG_SUPPORT_TWS
	int flags;

	flags = btsrv_set_negative_prio();
	btsrv_tws_set_input_stream(stream, true);
	btsrv_revert_prio(flags);

	return 0;
#endif
	return 0;
}

int btif_tws_set_sco_input_stream(io_stream_t stream)
{
#ifdef CONFIG_SUPPORT_TWS
	int flags;

	flags = btsrv_set_negative_prio();
	btsrv_tws_set_sco_input_stream(stream);
	btsrv_revert_prio(flags);

	return 0;
#endif
	return 0;
}

tws_runtime_observer_t *btif_tws_get_tws_observer(void)
{
#ifdef CONFIG_SUPPORT_TWS
	int flags;
	tws_runtime_observer_t *observer;

	flags = btsrv_set_negative_prio();
	observer = btsrv_tws_monitor_get_observer();
	btsrv_revert_prio(flags);

	return observer;
#else
	return NULL;
#endif
}

int btif_tws_send_command(uint8_t *data, int len)
{
#ifdef CONFIG_SUPPORT_TWS
	int ret, flags;

	flags = btsrv_set_negative_prio();
	ret = bsrv_tws_send_user_command(data, len);
	btsrv_revert_prio(flags);

	return ret;
#else
	return 0;
#endif
}

int btif_tws_send_command_sync(uint8_t *data, int len)
{
#ifdef CONFIG_SUPPORT_TWS
	int ret, flags;

	flags = btsrv_set_negative_prio();
	ret = bsrv_tws_send_user_command_sync(data, len);
	btsrv_revert_prio(flags);

	return ret;
#else
	return 0;
#endif
}

uint32_t btif_tws_get_bt_clock(void)
{
#ifdef CONFIG_SUPPORT_TWS
	int flags;
	uint32_t clock;

	flags = btsrv_set_negative_prio();
	clock = btsrv_tws_get_bt_clock();
	btsrv_revert_prio(flags);

	return clock;
#else
	return 0;
#endif
}

int btif_tws_set_bt_local_play(bool bt_play, bool local_play)
{
#ifdef CONFIG_SUPPORT_TWS
	int ret, flags;

	flags = btsrv_set_negative_prio();
	ret = btsrv_tws_set_bt_local_play(bt_play, local_play);
	btsrv_revert_prio(flags);

	return ret;
#endif
	return 0;
}

void btif_tws_update_tws_mode(uint8_t tws_mode, uint8_t drc_mode)
{
#ifdef CONFIG_SUPPORT_TWS
	int flags;

	flags = btsrv_set_negative_prio();
	btsrv_tws_updata_tws_mode(tws_mode, drc_mode);
	btsrv_revert_prio(flags);
#endif
}

bool btif_tws_check_is_woodpecker(void)
{
#ifdef CONFIG_SUPPORT_TWS
	int flags;
	bool ret;

	flags = btsrv_set_negative_prio();
	ret = btsrv_tws_check_is_woodpecker();
	btsrv_revert_prio(flags);

	return ret;
#else
	/* Not support tws, self as woodpecker */
	return true;
#endif
}

bool btif_tws_check_support_feature(uint32_t feature)
{
#ifdef CONFIG_SUPPORT_TWS
	int flags;
	bool ret;

	flags = btsrv_set_negative_prio();
	ret = btsrv_tws_check_support_feature(feature);
	btsrv_revert_prio(flags);

	return ret;
#else
	/* Not support tws,, self as support */
	return true;
#endif
}

void btif_tws_set_codec(uint8_t codec)
{
#ifdef CONFIG_SUPPORT_TWS
	int flags;

	flags = btsrv_set_negative_prio();
	btsrv_tws_set_codec(codec);
	btsrv_revert_prio(flags);
#endif
}
