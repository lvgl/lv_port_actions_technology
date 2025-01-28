# Copyright(C) 2020 Actions Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_AUDIO_OUT_DAC_SUPPORT phy_audio_dac.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_OUT_I2STX_SUPPORT phy_audio_i2stx.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT phy_audio_spdiftx.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_IN_ADC_SUPPORT phy_audio_adc.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_IN_I2SRX_SUPPORT phy_audio_i2srx.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_IN_SPDIFRX_SUPPORT phy_audio_spdifrx.c)
