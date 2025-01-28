# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(
    stream.c
)

zephyr_library_sources_ifdef(CONFIG_FILE_STREAM
    fstream.c
)

zephyr_library_sources_ifdef(CONFIG_MULTI_FILE_STREAM
    multi_fstream.c
)

zephyr_library_sources_ifdef(CONFIG_LOOP_FSTREAM
    loop_fstream.c
)

zephyr_library_sources_ifdef(CONFIG_NET_STREAM
    netstream.c
)

zephyr_library_sources_ifdef(CONFIG_BUFFER_STREAM
    bufferstream.c
)

zephyr_library_sources_ifdef(CONFIG_CACHE_STREAM
    psramstream.c
)

zephyr_library_sources_ifdef(CONFIG_CLONE_STREAM
    clonestream.c
)

zephyr_library_sources_ifdef(CONFIG_RINGBUFF_STREAM
    ringbuff_stream.c
)

zephyr_library_sources_ifdef(CONFIG_ZERO_STREAM
    zero_stream.c
)
