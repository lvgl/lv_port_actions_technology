# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_PDMA_ACTS	pdma_acts.c)
zephyr_library_sources_ifdef(CONFIG_VDMA_ACTS	vdma_acts.c)
