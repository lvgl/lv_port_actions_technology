/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BT_WATCH_SRC_LAUNCHER_GPS_VIEW_H_
#define BT_WATCH_SRC_LAUNCHER_GPS_VIEW_H_

#include <msg_manager.h>

#ifdef CONFIG_GPS_MANAGER
#include "gps_manager.h"
#endif

typedef struct gps_view_presenter {
#ifdef CONFIG_GPS_MANAGER
	gps_res_t* (*get_gps_data)(void);
#endif
    void (*reserved)(void);

} gps_view_presenter_t;

#endif
