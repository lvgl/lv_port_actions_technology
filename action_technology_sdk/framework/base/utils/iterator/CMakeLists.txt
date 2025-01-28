# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(
    iterator.c
)

zephyr_library_sources_ifdef(CONFIG_FILE_ITERATOR
    file_plist_iterator.c
)
