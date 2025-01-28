# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	.
)

target_sources(app PRIVATE
	third_party_app_ui.c
	third_party_app_view.c
	lvgx_applet_view.c
)
