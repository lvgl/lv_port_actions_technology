# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

target_sources(app PRIVATE usp_baudrate.c)
target_sources(app PRIVATE usp_communication.c)
target_sources(app PRIVATE usp_connect.c)
target_sources(app PRIVATE usp_disconnect.c)
target_sources(app PRIVATE usp_inquiry.c)
target_sources(app PRIVATE usp_open.c)
target_sources(app PRIVATE usp_protocol_fsm.c)
target_sources(app PRIVATE usp_protocol_init.c)
target_sources(app PRIVATE usp_transfer.c)
target_sources(app PRIVATE usp_command.c)
