#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_ACTIONS_PRINTK_DMA cbuf.c printk_dma.c)
if(CONFIG_SOC_LEOPARD) 
zephyr_sources_ifdef(CONFIG_ACTIONS_ARM_MPU leopard_mpu_arm.c)
else() 
zephyr_sources_ifdef(CONFIG_ACTIONS_ARM_MPU act_mpu_arm.c)
endif()
zephyr_sources_ifdef(CONFIG_KERNEL_SHOW_STACK show_thread.c)

