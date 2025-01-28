# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_HOTPLUG
    hotplug_manager.c
)
zephyr_library_sources_ifdef(CONFIG_LINEIN_HOTPLUG
    hotplug_linein.c
)
zephyr_library_sources_ifdef(CONFIG_CARD_HOTPLUG
    hotplug_card.c
)
zephyr_library_sources_ifdef(CONFIG_CHARGER_HOTPLUG
    hotplug_charger.c
)
zephyr_library_sources_ifdef(CONFIG_USB_HOTPLUG
    hotplug_usb.c
)
