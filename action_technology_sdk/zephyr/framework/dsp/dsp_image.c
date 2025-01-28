/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <os_common_api.h>
#include <drivers/dsp.h>
#include <string.h>
#include <mem_manager.h>
#include <sdfs.h>
#ifdef CONFIG_LOAD_IMAGE_FROM_FS
#include <fs/fs.h>
#endif /* LOAD_IMAGE_FROM_FS */

const struct dsp_imageinfo *dsp_create_image(const char *name)
{
#ifdef CONFIG_LOAD_IMAGE_FROM_FS
	char dsp_image_path[32];
#endif

	struct dsp_imageinfo *image = mem_malloc(sizeof(*image));
	if (image == NULL)
		return NULL;

#ifdef CONFIG_LOAD_IMAGE_FROM_FS
	snprintf(dsp_image_path, sizeof(dsp_image_path), "%s%s", CONFIG_DSP_IMAGE_PATH, name);
	if (fs_open(&image->filp, dsp_image_path, FS_O_READ)) {
		SYS_LOG_ERR("cannot find dsp image \"%s\"", dsp_image_path);
		image->size = 0;
		mem_free(image);
		return NULL;
	}
	image->size = 0x1000;
#else
	if (sd_fmap(name, (void **)&image->ptr, (int *)&image->size)) {
		SYS_LOG_ERR("cannot find dsp image \"%s\"", name);
		mem_free(image);
		return NULL;
	}
#endif

	image->name = name;

	return image;
}

void dsp_free_image(const struct dsp_imageinfo *image)
{
#ifdef CONFIG_LOAD_IMAGE_FROM_FS
	if (fs_close((struct fs_file_t *)&image->filp)) {
		SYS_LOG_ERR("cannot close file \"%s\"", image->name);
	}
#endif
	mem_free((void*)image);
}


