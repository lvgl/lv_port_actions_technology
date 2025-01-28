#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#
zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_SENSOR_HAL sensor_devices.c)
zephyr_sources_ifdef(CONFIG_SENSOR_ACC_LIS2DW12 sensor_acc_lis2dw12.c)
zephyr_sources_ifdef(CONFIG_SENSOR_ACC_STK8321 sensor_acc_stk8321.c)
zephyr_sources_ifdef(CONFIG_SENSOR_ACC_SC7A20 sensor_acc_sc7a20.c)
zephyr_sources_ifdef(CONFIG_SENSOR_ACC_QMA6100 sensor_acc_qma6100.c)
zephyr_sources_ifdef(CONFIG_SENSOR_HR_GH3011 sensor_hr_gh3011.c)
zephyr_sources_ifdef(CONFIG_SENSOR_HR_VC9201 sensor_hr_vc9201.c)
zephyr_sources_ifdef(CONFIG_SENSOR_HR_VC9202 sensor_hr_vc9202.c)
zephyr_sources_ifdef(CONFIG_SENSOR_HR_HX3605 sensor_hr_hx3605.c)
zephyr_sources_ifdef(CONFIG_SENSOR_HR_HX3690 sensor_hr_hx3690.c)
zephyr_sources_ifdef(CONFIG_SENSOR_MAG_MMC5603NJ sensor_mag_mmc5603nj.c)
zephyr_sources_ifdef(CONFIG_SENSOR_BARO_ICP10125 sensor_baro_icp10125.c)
