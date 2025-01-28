#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_ACTIONS_PRINTK_DMA cbuf.c printk_dma.c)
zephyr_sources_ifdef(CONFIG_KERNEL_SHOW_STACK show_thread.c)

