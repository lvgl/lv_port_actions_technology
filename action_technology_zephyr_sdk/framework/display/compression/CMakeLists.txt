# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(./)
zephyr_include_directories(./lz4/)
zephyr_include_directories(./rle/)
zephyr_library_sources(lz4/lz4.c lz4/lz4hc.c spress.c  rle/rle.c  tile_cache.c pic_compress.c)
