/*
 * Copyright (c) 2016 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt srv pnp info api interface
 */

#define SYS_LOG_DOMAIN "btif_pnp_info"
#include "btsrv_os_common.h"
#include "btsrv_inner.h"


int btif_pnp_info_register_processer(void)
{
	int ret = 0;
	SYS_LOG_INF("btif_pnp_info_register_processer\n");

	ret |= btsrv_register_msg_processer(MSG_BTSRV_PNP, &btsrv_pnp_info_process);
	return ret;
}


int btif_pnp_info_search_start(btsrv_pnp_info_callback cb)
{
	SYS_LOG_INF("btif_pnp_info_search_start\n");
	return btsrv_function_call(MSG_BTSRV_PNP, MSG_BTSRV_PNP_INFO_START, cb);
}

int btif_pnp_info_search_stop(void)
{
	SYS_LOG_INF("btif_pnp_info_search_stop\n");
	return btsrv_function_call(MSG_BTSRV_PNP, MSG_BTSRV_PNP_INFO_STOP, NULL);
}
