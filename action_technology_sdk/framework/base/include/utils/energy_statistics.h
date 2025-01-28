/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file energy statistics interface
 */
#include <stdint.h>
#ifndef __ENERGY_STATISTICS_H__
#define __ENERGY_STATISTICS_H__

/* structure to set energy filter param */
typedef struct {
	uint8_t enable;
	uint16_t threshold;
	/* attack time(ms) after energy below threshold */
	uint16_t attack_time;
	/* release time(ms) after energy above threshold */
	uint16_t release_time;
}energy_filter_t;

uint32_t energy_statistics(const short *pcm_data, uint32_t samples_cnt);
#endif