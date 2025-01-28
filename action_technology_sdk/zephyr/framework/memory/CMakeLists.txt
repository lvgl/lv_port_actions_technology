#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_sources(mem_cache.c)
zephyr_sources_ifdef(CONFIG_MEMORY_SLABS mem_slabes.c)
