# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
	.
	inc
)

target_sources(app PRIVATE 
	ui_sim.c
	ui_sim_menu.c
	ui_sim_tele.c
)
