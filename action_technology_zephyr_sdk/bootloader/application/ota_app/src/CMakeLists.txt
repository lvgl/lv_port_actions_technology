# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	.
	include
)

target_sources(app PRIVATE main.c)
add_subdirectory_ifdef(CONFIG_OTA_RECOVERY_APP ota_recovery)

