/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <board_cfg.h>
#include <display/display_composer.h>

#ifdef CONFIG_PANEL_FULL_SCREEN_OPT_AREA
static const ui_region_t fs_opt_areas[] = CONFIG_PANEL_FULL_SCREEN_OPT_AREA;
#endif

const ui_region_t * display_composer_opt_full_screen_areas(int *num_regions)
{
#ifdef CONFIG_PANEL_FULL_SCREEN_OPT_AREA
	*num_regions = ARRAY_SIZE(fs_opt_areas);
	return fs_opt_areas;
#else
	*num_regions = 0;
	return NULL;
#endif
}
