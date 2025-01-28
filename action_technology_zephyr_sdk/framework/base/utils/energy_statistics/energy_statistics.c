/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  energy statistics interface
 */
 
#include <stdint.h>
#include "energy_statistics.h"

uint32_t energy_statistics(const short *pcm_data, uint32_t samples_cnt)
{
	uint32_t sample_value = 0, i;
	int32_t temp_value;

	for (i = 0; i < samples_cnt; i++)
	{
		if (pcm_data[i] < 0)
			temp_value = 0 - pcm_data[i];
		else
			temp_value = pcm_data[i];
		sample_value += (uint32_t)temp_value;
	}

	return (sample_value / samples_cnt);
}
