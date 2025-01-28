# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CJSON_LIB)

zephyr_include_directories(.)

zephyr_sources(
	cJSON.c
)

endif()