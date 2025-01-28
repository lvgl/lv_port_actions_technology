
/*
 * Copyright (c) 2019 Actions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Audio_Sink descriptors (Device, Configuration, Interface ...)
 */

#ifndef __USB_AUDIO_SINK_DESC_H__
#define __USB_AUDIO_SINK_DESC_H__

#include <usb/usb_common.h>
#include <usb/class/usb_audio.h>

/* max upload packet for USB audio sink device */
#define RESOLUTION	CONFIG_USB_AUDIO_SINK_RESOLUTION
#define SUB_FRAME_SIZE	(RESOLUTION >> 3)
#define MAX_DOWNLOAD_PACKET       (((CONFIG_USB_AUDIO_SINK_SAMPLE_RATE / 1000) * RESOLUTION * CONFIG_USB_AUDIO_SINK_DOWNLOAD_CHANNEL_NUM)>>3)

#define FEATURE_UNIT_INDEX1	0x0300

/* Macro define*/
#define LOW_BYTE(x)  ((x) & 0xFF)
#define HIGH_BYTE(x) ((x) >> 8)

#define	SAM_LOW_BYTE(x)	(u8_t)(x)
#define	SAM_MIDDLE_BYTE(x)	(u8_t)(x >> 8)
#define	SAM_HIGH_BYTE(x)	(u8_t)(x >> 16)

static const u8_t usb_audio_sink_fs_descriptor[] = {
	/* Device Descriptor */
	USB_DEVICE_DESC_SIZE,	/* bLength */
	USB_DEVICE_DESC,	/* bDescriptorType */
	LOW_BYTE(USB_2_0),/* bcdUSB */
	HIGH_BYTE(USB_2_0),
	0x00,	/* bDeviceClass */
	0x00,	/* bDeviceSubClass */
	0x00,	/* bDeviceProtocol */
	MAX_PACKET_SIZE0,	/* bMaxPacketSize0:(64) Bytes */
	LOW_BYTE(CONFIG_USB_AUDIO_SINK_VID),	/* idVendor */
	HIGH_BYTE(CONFIG_USB_AUDIO_SINK_VID),
	LOW_BYTE(CONFIG_USB_AUDIO_SINK_PID),	/* Product */
	HIGH_BYTE(CONFIG_USB_AUDIO_SINK_PID),
	LOW_BYTE(BCDDEVICE_RELNUM),	/* bcdDevice */
	HIGH_BYTE(BCDDEVICE_RELNUM),
	0x01,	/* iManufacturer */
	0x02,	/* iProduct */
	0x03,	/* iSerialNumber */
	0x01,	/* bNumConfigurations */

	/* Configuration Descriptor */
	USB_CONFIGURATION_DESC_SIZE,	/* bLength */
	USB_CONFIGURATION_DESC,	/* bDescriptorType */
	LOW_BYTE(0x006E),	/* wTotalLength */
	HIGH_BYTE(0x006E),
	0x02,	/* bNumInterfaces */
	0x01,	/* bConfigurationValue */
	0x00,	/* iConfiguration */
	USB_CONFIGURATION_ATTRIBUTES,	/* bmAttributes: Bus Powered */
	MAX_LOW_POWER,	/* MaxPower: 100 mA */

	/* Interface Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,	/* bDescriptorType */
	0x00,	/* bInterfaceNumber */
	0x00,	/* bAlternateSetting */
	0x00,	/* bNumEndpoints */
	/* bInterfaceClass: Audio Interface Class */
	USB_CLASS_AUDIO,
	/* bInterfaceSubClass:  Audio Control Interface SubClass */
	USB_SUBCLASS_AUDIOCONTROL,
	0x00,	/* bInterfaceProtocol */
	0x00,	/* iInterface */

	/* Audio Control Interface Header Descriptor */
	0x09,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_HEADER,	/* bDescriptorSubtype */
	LOW_BYTE(0x0100),	/* bcdADC */
	HIGH_BYTE(0x0100),
	LOW_BYTE(0x0028),	/* wTotalLength */
	HIGH_BYTE(0x0028),
	0x01,	/* bInCollection */
	0x01,	/* baInterfaceNr[1] */

	/* Audio Control Input Terminal Descriptor */
	UAC_DT_INPUT_TERMINAL_SIZE,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_INPUT_TERMINAL,	/* bDescriptorSubtype */
	0x01,	/* bTerminalID */
	LOW_BYTE(UAC_TERMINAL_STREAMING),	/* wTerminalType: USB streaming */
	HIGH_BYTE(UAC_TERMINAL_STREAMING),
	0x00,	/* bAssocTerminal*/
	CONFIG_USB_AUDIO_SINK_DOWNLOAD_CHANNEL_NUM,	/* bNrChannels*/
	LOW_BYTE(0x0003),	/* wChannelConfig */
	HIGH_BYTE(0x0003),
	0x00,	/* iChannelNames */
	0x00,	/* iTerminal */

	/* Audio Control Feature Unit Descriptor */
	0x0A,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_FEATURE_UNIT,	/* bDescriptorSubtype */
	HIGH_BYTE(FEATURE_UNIT_INDEX1),	/* bUnitID */
	0x01,	/* bSourceID */
	0x01,	/* bControlSize */
	03,		/* bmaControls[0] */
	03,		/* bmaControls[1] */
	03,		/* bmaControls[2] */
	LOW_BYTE(FEATURE_UNIT_INDEX1),	/* iFeature */

	/* Audio Control Output Terminal Descriptor */
	UAC_DT_OUTPUT_TERMINAL_SIZE,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_OUTPUT_TERMINAL,	/* bDescriptorSubtype */
	0x02,	/* bTerminalID */
	LOW_BYTE(UAC_OUTPUT_TERMINAL_SPEAKER),	/* wTerminalType:Speaker */
	HIGH_BYTE(UAC_OUTPUT_TERMINAL_SPEAKER),
	0x00,	/* bAssocTerminal */
	0x03,	/* bSourceID */
	0x00,	/* iTerminal */

	/* Interface Descripto */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,	/* bDescriptorType */
	0x01,	/* bInterfaceNumber */
	0x00,	/* bAlternateSetting */
	0x00,	/* bNumEndpoints */
	/* bInterfaceClass: Audio Interface Class */
	USB_CLASS_AUDIO,
	/* bInterfaceSubClass: Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,	/* bInterfaceProtocol */
	0x00,	/* iInterface */

	/* Interface Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,	/* bDescriptorType */
	0x01,	/* bInterfaceNumber */
	0x01,	/* bAlternateSetting */
	0x01,	/* bNumEndpoints */
	/* bInterfaceClass: Audio Interface Class */
	USB_CLASS_AUDIO,
	/* bInterfaceSubClass: Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,	/* bInterfaceProtocol */
	0x00,	/* iInterface */

	/* Audio Streaming Class Specific Interface Descriptor */
	UAC_DT_AS_HEADER_SIZE,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_AS_GENERAL,	/* bDescriptorSubtype */
	0x01,	/* bTerminalLink */
	0x01,	/* bDelay */
	LOW_BYTE(UAC_FORMAT_TYPE_I_PCM),	/* wFormatTag: PCM */
	HIGH_BYTE(UAC_FORMAT_TYPE_I_PCM),

	/* Audio Streaming Format Type Descriptor */
	0x0B,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_FORMAT_TYPE,	/* bDescriptorSubtype */
	UAC_FORMAT_TYPE_I,	/* bFormatType */
	CONFIG_USB_AUDIO_SINK_DOWNLOAD_CHANNEL_NUM,	/* bNrChannels */
	SUB_FRAME_SIZE,	/* bSubframeSize */
	RESOLUTION,	/* bBitResolution */
	0x01,	/* bSamFreqType */
	SAM_LOW_BYTE(CONFIG_USB_AUDIO_SINK_SAMPLE_RATE),	/* tSamFreq[n] */
	SAM_MIDDLE_BYTE(CONFIG_USB_AUDIO_SINK_SAMPLE_RATE),
	SAM_HIGH_BYTE(CONFIG_USB_AUDIO_SINK_SAMPLE_RATE),

	/* Endpoint Descriptor */
	0x09,	/* bLength */
	USB_ENDPOINT_DESC,	/* bDescriptorType */
	/* bEndpointAddress: Direction: OUT - EndpointID: n */
	CONFIG_USB_AUDIO_SINK_OUT_EP_ADDR,
	/* bmAttributes: Isochronous Transfer Type */
	0x0D,
	LOW_BYTE(MAX_DOWNLOAD_PACKET),	/* wMaxPacketSize: n bytes */
	HIGH_BYTE(MAX_DOWNLOAD_PACKET),
	LOW_BYTE(CONFIG_USB_AUDIO_SINK_OUT_EP_FS_INTERVAL),	/* wInterval */
	HIGH_BYTE(CONFIG_USB_AUDIO_SINK_OUT_EP_FS_INTERVAL),
	0x00,	/* bSyncAddress */

	/* Audio Streaming Class Specific Audio Data Endpoint Descriptor */
	UAC_ISO_ENDPOINT_DESC_SIZE,	/* bLength */
	CS_ENDPOINT,	/* bDescriptorType */
	UAC_EP_GENERAL,	/* bDescriptorSubtype */
	0x00,	/* bmAttributes */
	0x00,	/* bLockDelayUnits */
	LOW_BYTE(0x0001),	/* wLockDelay */
	HIGH_BYTE(0x0001),
};

static const u8_t usb_audio_sink_hs_descriptor[] = {
	/* Device Descriptor */
	USB_DEVICE_DESC_SIZE,	/* bLength */
	USB_DEVICE_DESC,	/* bDescriptorType */
	LOW_BYTE(USB_2_0),/* bcdUSB */
	HIGH_BYTE(USB_2_0),
	0x00,	/* bDeviceClass */
	0x00,	/* bDeviceSubClass */
	0x00,	/* bDeviceProtocol */
	MAX_PACKET_SIZE0,	/* bMaxPacketSize0:(64) Bytes */
	LOW_BYTE(CONFIG_USB_AUDIO_SINK_VID),	/* idVendor */
	HIGH_BYTE(CONFIG_USB_AUDIO_SINK_VID),
	LOW_BYTE(CONFIG_USB_AUDIO_SINK_PID),	/* Product */
	HIGH_BYTE(CONFIG_USB_AUDIO_SINK_PID),
	LOW_BYTE(BCDDEVICE_RELNUM),	/* bcdDevice */
	HIGH_BYTE(BCDDEVICE_RELNUM),
	0x01,	/* iManufacturer */
	0x02,	/* iProduct */
	0x03,	/* iSerialNumber */
	0x01,	/* bNumConfigurations */

	/* Configuration Descriptor */
	USB_CONFIGURATION_DESC_SIZE,	/* bLength */
	USB_CONFIGURATION_DESC,	/* bDescriptorType */
	LOW_BYTE(0x006E),	/* wTotalLength */
	HIGH_BYTE(0x006E),
	0x02,	/* bNumInterfaces */
	0x01,	/* bConfigurationValue */
	0x00,	/* iConfiguration */
	USB_CONFIGURATION_ATTRIBUTES,	/* bmAttributes: Bus Powered */
	MAX_LOW_POWER,	/* MaxPower: 100 mA */

	/* Interface Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,	/* bDescriptorType */
	0x00,	/* bInterfaceNumber */
	0x00,	/* bAlternateSetting */
	0x00,	/* bNumEndpoints */
	/* bInterfaceClass: Audio Interface Class */
	USB_CLASS_AUDIO,
	/* bInterfaceSubClass:  Audio Control Interface SubClass */
	USB_SUBCLASS_AUDIOCONTROL,
	0x00,	/* bInterfaceProtocol */
	0x00,	/* iInterface */

	/* Audio Control Interface Header Descriptor */
	0x09,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_HEADER,	/* bDescriptorSubtype */
	LOW_BYTE(0x0100),	/* bcdADC */
	HIGH_BYTE(0x0100),
	LOW_BYTE(0x0028),	/* wTotalLength */
	HIGH_BYTE(0x0028),
	0x01,	/* bInCollection */
	0x01,	/* baInterfaceNr[1] */

	/* Audio Control Input Terminal Descriptor */
	UAC_DT_INPUT_TERMINAL_SIZE,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_INPUT_TERMINAL,	/* bDescriptorSubtype */
	0x01,	/* bTerminalID */
	LOW_BYTE(UAC_TERMINAL_STREAMING),	/* wTerminalType: USB streaming */
	HIGH_BYTE(UAC_TERMINAL_STREAMING),
	0x00,	/* bAssocTerminal*/
	CONFIG_USB_AUDIO_SINK_DOWNLOAD_CHANNEL_NUM,	/* bNrChannels*/
	LOW_BYTE(0x0003),	/* wChannelConfig */
	HIGH_BYTE(0x0003),
	0x00,	/* iChannelNames */
	0x00,	/* iTerminal */

	/* Audio Control Feature Unit Descriptor */
	0x0A,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_FEATURE_UNIT,	/* bDescriptorSubtype */
	HIGH_BYTE(FEATURE_UNIT_INDEX1),	/* bUnitID */
	0x01,	/* bSourceID */
	0x01,	/* bControlSize */
	03,		/* bmaControls[0] */
	03,		/* bmaControls[1] */
	03,		/* bmaControls[2] */
	LOW_BYTE(FEATURE_UNIT_INDEX1),	/* iFeature */

	/* Audio Control Output Terminal Descriptor */
	UAC_DT_OUTPUT_TERMINAL_SIZE,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_OUTPUT_TERMINAL,	/* bDescriptorSubtype */
	0x02,	/* bTerminalID */
	LOW_BYTE(UAC_OUTPUT_TERMINAL_SPEAKER),	/* wTerminalType:Speaker */
	HIGH_BYTE(UAC_OUTPUT_TERMINAL_SPEAKER),
	0x00,	/* bAssocTerminal */
	0x03,	/* bSourceID */
	0x00,	/* iTerminal */

	/* Interface Descripto */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,	/* bDescriptorType */
	0x01,	/* bInterfaceNumber */
	0x00,	/* bAlternateSetting */
	0x00,	/* bNumEndpoints */
	/* bInterfaceClass: Audio Interface Class */
	USB_CLASS_AUDIO,
	/* bInterfaceSubClass: Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,	/* bInterfaceProtocol */
	0x00,	/* iInterface */

	/* Interface Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,	/* bDescriptorType */
	0x01,	/* bInterfaceNumber */
	0x01,	/* bAlternateSetting */
	0x01,	/* bNumEndpoints */
	/* bInterfaceClass: Audio Interface Class */
	USB_CLASS_AUDIO,
	/* bInterfaceSubClass: Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,	/* bInterfaceProtocol */
	0x00,	/* iInterface */

	/* Audio Streaming Class Specific Interface Descriptor */
	UAC_DT_AS_HEADER_SIZE,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_AS_GENERAL,	/* bDescriptorSubtype */
	0x01,	/* bTerminalLink */
	0x01,	/* bDelay */
	LOW_BYTE(UAC_FORMAT_TYPE_I_PCM),	/* wFormatTag: PCM */
	HIGH_BYTE(UAC_FORMAT_TYPE_I_PCM),

	/* Audio Streaming Format Type Descriptor */
	0x0B,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_FORMAT_TYPE,	/* bDescriptorSubtype */
	UAC_FORMAT_TYPE_I,	/* bFormatType */
	CONFIG_USB_AUDIO_SINK_DOWNLOAD_CHANNEL_NUM,	/* bNrChannels */
	SUB_FRAME_SIZE,	/* bSubframeSize */
	RESOLUTION,	/* bBitResolution */
	0x01,	/* bSamFreqType */
	SAM_LOW_BYTE(CONFIG_USB_AUDIO_SINK_SAMPLE_RATE),	/* tSamFreq[n] */
	SAM_MIDDLE_BYTE(CONFIG_USB_AUDIO_SINK_SAMPLE_RATE),
	SAM_HIGH_BYTE(CONFIG_USB_AUDIO_SINK_SAMPLE_RATE),

	/* Endpoint Descriptor */
	0x09,	/* bLength */
	USB_ENDPOINT_DESC,	/* bDescriptorType */
	/* bEndpointAddress: Direction: OUT - EndpointID: n */
	CONFIG_USB_AUDIO_SINK_OUT_EP_ADDR,
	/* bmAttributes: Isochronous Transfer Type */
	0x0D,
	LOW_BYTE(MAX_DOWNLOAD_PACKET),	/* wMaxPacketSize: n bytes */
	HIGH_BYTE(MAX_DOWNLOAD_PACKET),
	LOW_BYTE(CONFIG_USB_AUDIO_SINK_OUT_EP_HS_INTERVAL),	/* wInterval */
	HIGH_BYTE(CONFIG_USB_AUDIO_SINK_OUT_EP_HS_INTERVAL),
	0x00,	/* bSyncAddress */

	/* Audio Streaming Class Specific Audio Data Endpoint Descriptor */
	UAC_ISO_ENDPOINT_DESC_SIZE,	/* bLength */
	CS_ENDPOINT,	/* bDescriptorType */
	UAC_EP_GENERAL,	/* bDescriptorSubtype */
	0x00,	/* bmAttributes */
	0x00,	/* bLockDelayUnits */
	LOW_BYTE(0x0001),	/* wLockDelay */
	HIGH_BYTE(0x0001),
};

#endif /* __USB_AUDIO_SINK_DESC_H__ */
