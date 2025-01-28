#
# Copyright (c) 2020 Actions Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Actions
#

zephyr_sources(
	arithmetic.c
	arithmetic_storage_io.c
)

zephyr_sources_ifdef(CONFIG_DECODER_PCM codec/pcm_decoder.c)
# zephyr_sources_ifdef(CONFIG_ENCODER_PCM codec/pcm_encoder.c)

