#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_include_directories(
	.
)

zephyr_sources(
	sensor_algo_common.c
)

if(CONFIG_SENSOR_ALGO_MOTION_CYWEE)
  add_subdirectory(SensorAlgoMotion_Cywee)
elseif(CONFIG_SENSOR_ALGO_MOTION_SILAN)
  add_subdirectory(SensorAlgoMotion_Silan)
else()
  add_subdirectory(SensorAlgoMotion)
endif()

if(CONFIG_SENSOR_ALGO_HR_GH30X)
  add_subdirectory(SensorAlgoHR_GH30x)
elseif(CONFIG_SENSOR_ALGO_HR_VCARE)
  add_subdirectory(SensorAlgoHR_Vcare)
elseif(CONFIG_SENSOR_ALGO_HR_VCARE_VC9202)
  add_subdirectory(SensorAlgoHR_Vcare_VC9202)
elseif(CONFIG_SENSOR_ALGO_HR_HX3605)
  add_subdirectory(SensorAlgoHR_HX3605)
elseif(CONFIG_SENSOR_ALGO_HR_HX3690)
  add_subdirectory(SensorAlgoHR_HX3690)
else()
  add_subdirectory(SensorAlgoHR)
endif()
