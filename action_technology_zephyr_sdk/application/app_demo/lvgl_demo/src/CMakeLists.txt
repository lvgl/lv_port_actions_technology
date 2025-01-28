# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	.
)

target_sources(app PRIVATE
	main.c
	app_msg.c
	ota_app.c
)
