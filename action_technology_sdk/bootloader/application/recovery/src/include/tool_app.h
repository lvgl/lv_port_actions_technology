/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TOOL_APP_H__
#define __TOOL_APP_H__

/**
 * @brief initialize pc tool
 *
 * This routine provides initialize pc tool. After that, pc tools become usable.
 *
 * @return 0 if succeed, otherwise failed.
 */
int tool_init(char *type);

/**
 * @brief deinitialize pc tool
 *
 * This routine provides deinitialize pc tool. After that, pc tools become unusable.
 */
void tool_deinit(void);

int tool_att_connect_try(void);

#define TOOL_DEV_TYPE_NULL   0
#define TOOL_DEV_TYPE_USB    1
#define TOOL_DEV_TYPE_UART0  2
#define TOOL_DEV_TYPE_UART1  3
#define TOOL_DEV_TYPE_CARD   4

int tool_get_dev_type(void);
const struct device * tool_get_dev_device(void);

#endif /* __TOOL_APP_H__ */
