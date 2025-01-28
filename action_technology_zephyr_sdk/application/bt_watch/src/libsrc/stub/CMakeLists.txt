# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

target_sources(app PRIVATE stub_init.c)
target_sources(app PRIVATE stub_open.c)
target_sources(app PRIVATE stub_close.c)
target_sources(app PRIVATE stub_command.c)
target_sources(app PRIVATE stub_read.c)
target_sources(app PRIVATE stub_write.c)
target_sources(app PRIVATE stub_baudrate.c)
