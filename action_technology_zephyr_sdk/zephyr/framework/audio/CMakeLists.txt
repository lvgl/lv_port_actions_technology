#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_sources_ifdef(CONFIG_AUDIO_IN audioin_hal.c)
zephyr_sources_ifdef(CONFIG_AUDIO_OUT audioout_hal.c)
