/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file fm manager interface
 */

#ifndef __FM_MANAGER_H__
#define __FM_MANAGER_H__
#include <stdint.h>
/**
 * @defgroup alarm_manager_apis alarm manager APIs
 * @ingroup system_apis
 * @{
 */
/**
 * @cond INTERNAL_HIDDEN
 */

int fm_manager_init(void);
int fm_rx_init(void *param);
int fm_rx_set_freq(uint32_t freq);
int fm_rx_set_mute(bool mute);
int fm_rx_check_freq(uint32_t freq);
int fm_tx_set_freq(uint32_t freq);
int fm_tx_get_freq(void);
int fm_rx_deinit(void);
int fm_manager_deinit(void);
/**
 * INTERNAL_HIDDEN @endcond
 */
/**
 * @} end defgroup alarm_manager_apis
 */
#endif
