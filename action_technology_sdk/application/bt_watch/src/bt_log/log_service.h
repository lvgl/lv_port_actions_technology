/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_LOG_LOG_SERVICE_H__
#define __BT_LOG_LOG_SERVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start bt log service
 *
 * @retval 0 on success else negative code.
 */
int bt_log_service_start(void);

/**
 * @brief Stop bt log service
 *
 * @retval 0 on success else negative code.
 */
int bt_log_service_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* __BT_LOG_LOG_SERVICE_H__ */
