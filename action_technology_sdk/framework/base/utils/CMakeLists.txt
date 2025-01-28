# Copyright (c) 2020 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_ACTS_RING_BUFFER acts_ringbuf)
add_subdirectory_ifdef(CONFIG_ACTS_EBERGY_STATISTICS energy_statistics)
add_subdirectory_ifdef(CONFIG_ITERATOR iterator)
add_subdirectory_ifdef(CONFIG_STREAM stream)
add_subdirectory(timeline)



