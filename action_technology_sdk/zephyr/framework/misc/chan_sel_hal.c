/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os_common_api.h>
#include <shell/shell.h>

#include "chan_sel_hal.h"

int chan_sel_device_inquiry(struct audio_chansel_data *data,char * dev_name)
{
	struct device * chan_dev;
	int ret = 0;

	chan_dev = (struct device *)device_get_binding(dev_name);

	ret = audio_chan_sel(chan_dev, data);

	return ret;

}
