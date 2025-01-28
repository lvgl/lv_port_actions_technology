# SPDX-License-Identifier: Apache-2.0

if (CONFIG_OTA_FOREGROUND)
target_sources(app PRIVATE ota_app_foreground.c)
target_sources_ifdef(CONFIG_UI_MANAGER app PRIVATE ota_app_view.c)
endif()

if (CONFIG_OTA_BACKGROUND)
target_sources(app PRIVATE ota_app_background.c)
target_sources_ifdef(CONFIG_UI_MANAGER app PRIVATE ota_msgbox_view.c)
endif()
