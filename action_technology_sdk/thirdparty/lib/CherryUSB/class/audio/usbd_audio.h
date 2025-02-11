/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef USBD_AUDIO_H
#define USBD_AUDIO_H

#include "usb_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	LOW_BYTE(x)		((x) & 0xFF)
#define	HIGH_BYTE(x)	(((x) >> 8)&0xFF)

/* volume configuration of max, min, resolution
 * Actions's mic db: -60~0
 */
#define	MAXIMUM_VOLUME	(0x0000)
#define	MINIMUM_VOLUME	(0xC3D8)
#define	RESOTION_VOLUME	(0x0001)

struct audio_entity_info {
    uint8_t bDescriptorSubtype;
    uint8_t bEntityId;
    uint8_t ep;
};

enum private_cmd_type {
	UAC_STANDARD_CMD,
	UAC_PC_TOOL_SWITCH_CMD, /* No-standard command */
};

#define LEN_MUTE	(1)
/* len of pc tool is 4, content: 0xcb,0x21,0xcb,0x22*/
#define LEN_PC_TOOL_SWITCH	(4)

/* Init audio interface driver */
struct usbd_interface *usbd_audio_init_intf(uint8_t busid, struct usbd_interface *intf,
                                            uint16_t uac_version,
                                            struct audio_entity_info *table,
                                            uint8_t num);
/**
 * @brief Callback function of audio no-standard command(host to device).
 *
 * @param[cmd_type]    standard or no standard command.
 * @param[cmd_len]     number of left/right/main channel.
 * @param[pstore_info]  volume value.
 */
typedef void (*usb_audio_private_cmd_cb)(uint8_t cmd_type, int *cmd_len, uint8_t *cmd_data);

void usbd_audio_open(uint8_t busid, uint8_t intf, uint8_t alt_setting);
void usbd_audio_close(uint8_t busid, uint8_t intf);

void usbd_audio_set_volume(uint8_t busid, uint8_t ep, uint8_t ch, int volume);
int usbd_audio_get_volume(uint8_t busid, uint8_t ep, uint8_t ch);
void usbd_audio_set_mute(uint8_t busid, uint8_t ep, uint8_t ch, bool mute);
bool usbd_audio_get_mute(uint8_t busid, uint8_t ep, uint8_t ch);
void usbd_audio_set_sampling_freq(uint8_t busid, uint8_t ep, uint32_t sampling_freq);
uint32_t usbd_audio_get_sampling_freq(uint8_t busid, uint8_t ep);
void usbd_audio_register_private_cmd_cb(usb_audio_private_cmd_cb cb);

void usbd_audio_get_sampling_freq_table(uint8_t busid, uint8_t ep, uint8_t **sampling_freq_table);

#ifdef __cplusplus
}
#endif

#endif /* USBD_AUDIO_H */
