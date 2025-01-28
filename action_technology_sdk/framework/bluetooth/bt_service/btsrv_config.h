/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file media service config interface
 */
#ifndef __BTSRV_CONFIG_H__
#define __BTSRV_CONFIG_H__

#define CONFIG_MAX_BT_NAME_LEN      32

#ifdef CONFIG_TWS
#define CONFIG_SUPPORT_TWS          1
#endif

#define CONFIG_MAX_A2DP_ENDPOINT	3

#define CONFIG_MAX_SPP_CHANNEL		4

#define CONFIG_MAX_PBAP_CONNECT		2

#define CONFIG_MAX_MAP_CONNECT		2

#define CONFIG_DEBUG_DATA_RATE      1
#endif
