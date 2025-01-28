# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_RBUF_LIB rbuf_core.c rbuf_pool.c rbuf_init.c)
