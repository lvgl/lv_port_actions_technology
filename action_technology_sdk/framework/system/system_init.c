/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system init file
 */
#include <os_common_api.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <srv_manager.h>


/*share stack for app thread */
//char __stack_noinit  __aligned(ARCH_STACK_PTR_ALIGN) share_stack_area[CONFIG_APP_STACKSIZE];

#ifdef CONFIG_MEDIA_SERVICE
/*stack for media service */
#ifdef CONFIG_SOC_NO_PSRAM
__in_section_unique(media.noinit.stack)
#endif
char  __aligned(ARCH_STACK_PTR_ALIGN) meidasrv_stack_area[CONFIG_MEDIASRV_STACKSIZE];

extern void media_service_main_loop(void * parama1, void * parama2, void * parama3);

SERVICE_DEFINE(media, \
				meidasrv_stack_area, CONFIG_MEDIASRV_STACKSIZE, \
				CONFIG_MEDIASRV_PRIORITY, BACKGROUND_APP, \
				NULL, NULL, NULL, \
				media_service_main_loop);
#endif


#ifdef CONFIG_BT_SERVICE
/*stack for bt service */
#ifdef CONFIG_SOC_NO_PSRAM
__in_section_unique(bthost.noinit.stack)
#endif
char __aligned(ARCH_STACK_PTR_ALIGN) btsrv_stack_area[CONFIG_BTSRV_STACKSIZE];

extern void bt_service_main_loop(void * parama1, void * parama2, void * parama3);
SERVICE_DEFINE(bluetooth, \
				btsrv_stack_area,	CONFIG_BTSRV_STACKSIZE, \
				CONFIG_BTSRV_PRIORITY, BACKGROUND_APP, \
				NULL, NULL, NULL, \
				bt_service_main_loop);
#endif

#ifdef CONFIG_BT_HCI
extern uint32_t libbtstack_version_dump(void);
extern uint32_t libbtservice_version_dump(void);
#ifdef CONFIG_BT_A2DP_TRS
extern uint32_t libbttrans_version_dump(void);
#endif
#endif

#ifdef CONFIG_MEDIA_SERVICE
extern uint32_t libmedia_version_dump(void);
#endif

#ifdef CONFIG_OTA_UPGRADE
extern uint32_t libota_version_dump(void);
#endif

#ifdef CONFIG_UI_MANAGER
extern uint32_t libdisplay_version_dump(void);
#endif

void system_library_version_dump(void)
{
#ifdef CONFIG_BT_HCI
#ifdef CONFIG_BT_A2DP_TRS
	libbttrans_version_dump();
#endif
#endif

#ifdef CONFIG_MEDIA_SERVICE
	libmedia_version_dump();
#endif

#ifdef CONFIG_OTA_UPGRADE
	libota_version_dump();
#endif

#ifdef CONFIG_UI_MANAGER
	libdisplay_version_dump();
#endif

	os_printk("\n");
}

