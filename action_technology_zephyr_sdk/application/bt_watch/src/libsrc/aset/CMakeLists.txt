# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

target_sources(app PRIVATE aset_fsm.c)
target_sources(app PRIVATE aset_init.c)
target_sources(app PRIVATE aset_read.c)
