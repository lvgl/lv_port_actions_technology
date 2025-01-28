# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_USB_AUDIO_SOURCE	usb_audio_source.c)
zephyr_library_sources_ifdef(CONFIG_USB_AUDIO_SINK	usb_audio_sink.c)
zephyr_library_sources_ifdef(CONFIG_USB_AUDIO_SOURCESINK	audio_sourcesink.c)