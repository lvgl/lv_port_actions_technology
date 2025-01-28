/*
 * Copyright (c) 1997-2015, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <zephyr.h>
#include <drivers/anc.h>
#include <soc_anc.h>
#include "anc_inner.h"
#include "anc_image.h"

int anc_acts_request_image(struct device *dev, const struct anc_imageinfo *image)
{
	uint32_t entry_point = UINT32_MAX;

	struct anc_acts_data *anc_data = dev->data;


	if (load_anc_image(image->ptr, image->size, &entry_point)) {
		printk("%s: cannot load anc image <%s>\n", __func__, image->name);
		return -EINVAL;
	}

	printk("%s: anc image <%s> loaded, entry_point=0x%x\n",
			__func__, image->name, entry_point);

	/* set ANC_VECTOR_ADDRESS */
	set_anc_vector_addr(entry_point);

	SYS_LOG_INF(" ANC_VCT_ADDR=0x%x 0x%x\n", sys_read32(ANC_VCT_ADDR), entry_point);
	memcpy(&anc_data->images, image, sizeof(*image));

	anc_data->images.entry_point = entry_point;
	return 0;
}

int anc_acts_release_image(struct device *dev)
{
	struct anc_acts_data *anc_data = dev->data;

	printk("%s: anc image <%s> free\n",
		__func__, anc_data->images.name);
	memset(&anc_data->images, 0, sizeof(anc_data->images));

	return 0;
}

