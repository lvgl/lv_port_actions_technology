/*
 * Copyright (c) 2019 Actions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
*/
/**
 * @file
 * @brief USB Audio_Source descriptors (Device, Configuration, Interface ...)
 */

#ifndef __USB_AUDIO_SOURCE_DESC_H__
#define __USB_AUDIO_SOURCE_DESC_H__
#include <usb/usb_common.h>

/* max upload packet for USB audio source device */
#define RESOLUTION	CONFIG_USB_AUDIO_SOURCE_RESOLUTION
#define SUB_FRAME_SIZE	(RESOLUTION >> 3)
#define MAX_UPLOAD_PACKET       (((CONFIG_USB_AUDIO_SOURCE_SAMPLE_RATE / 1000) * RESOLUTION * CONFIG_USB_AUDIO_SOURCE_UPLOAD_CHANNEL_NUM)>>3)

/* Macro define*/
#define LOW_BYTE(x)  ((x) & 0xFF)
#define HIGH_BYTE(x) ((x) >> 8)

#define	SAM_LOW_BYTE(x)	(u8_t)(x)
#define	SAM_MIDDLE_BYTE(x)	(u8_t)(x >> 8)
#define	SAM_HIGH_BYTE(x)	(u8_t)(x >> 16)

static const u8_t usb_audio_source_fs_descriptor[] = {
	/* Device Descriptor */
	USB_DEVICE_DESC_SIZE,	/* bLength */
	USB_DEVICE_DESC,	/* bDescriptorType */
	LOW_BYTE(USB_2_0),	/* bcdUSB */
	HIGH_BYTE(USB_2_0),
	0x00,	/* bDeviceClass */
	0x00,	/* bDeviceSubClass */
	0x00,	/* bDeviceProtocol */
	MAX_PACKET_SIZE0,	/* bMaxPacketSize0 = (64) Bytes */
	LOW_BYTE(CONFIG_USB_AUDIO_SOURCE_VID),	/* idVendor VID */
	HIGH_BYTE(CONFIG_USB_AUDIO_SOURCE_VID),
	LOW_BYTE(CONFIG_USB_AUDIO_SOURCE_PID),	/* idProduct PID */
	HIGH_BYTE(CONFIG_USB_AUDIO_SOURCE_PID),
	LOW_BYTE(BCDDEVICE_RELNUM),	/* bcdDevice */
	HIGH_BYTE(BCDDEVICE_RELNUM),
	0x01,	/* iManufacturer */
	0x02,	/* iProduct */
	0x03,	/* iSerialNumber */
	0x01,	/* bNumConfigurations */

	/* Configuration Descriptor */
	USB_CONFIGURATION_DESC_SIZE,	/* bLength */
	USB_CONFIGURATION_DESC,	/* bDescriptorType */
	LOW_BYTE(0x0062),	/* wTotalLength */
	HIGH_BYTE(0x0062),
	0x02,	/* bNumInterfaces */
	0x01,	/* bConfigurationValue */
	0x00,	/* iConfiguration */
	USB_CONFIGURATION_ATTRIBUTES,	/* bmAttributes */
	MAX_LOW_POWER,	/* MaxPower = 100 mA */

	/* Interface Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,	/* bDescriptorType */
	0x00,	/* bInterfaceNumber */
	0x00,	/* bAlternateSetting */
	0x00,	/* bNumEndpoints */
	USB_CLASS_AUDIO,	/* bInterfaceClass:  Audio Interface Class */
	USB_SUBCLASS_AUDIOCONTROL,	/* bInterface SubClass */
	0x00,	/* bInterfaceProtocol */
	0x00,	/* iInterface */

	/* Class-Specific AC Interface Descriptor */
	/* 1. Audio Control Interface Header Descriptor */
	0x09,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_HEADER,	/* bDescriptorSubtype */
	LOW_BYTE(0x0100),	/* bcdADC */
	HIGH_BYTE(0x0100),
	/* wTotalLength[30/0x1E] = len(header_desc[9] + ITD[12] + OTD[9]) */
	LOW_BYTE(0x001E),
	HIGH_BYTE(0x001E),
	0x01,	/* bInCollection */
	0x01,	/* baInterfaceNr[1] */

	/* Audio Control Input Terminal Descriptor */
	UAC_DT_INPUT_TERMINAL_SIZE,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_INPUT_TERMINAL,	/* bDescriptorSubtype */
	0x01,	/* bTerminalID */
	LOW_BYTE(UAC_INPUT_TERMINAL_MICROPHONE),	/* wTerminalType:(Microphone) */
	HIGH_BYTE(UAC_INPUT_TERMINAL_MICROPHONE),
	0x00,	/* bAssocTerminal */
	0x01,	/* One channe */
	/* wChannelConfig: Single channel does not set bit mapping */
	LOW_BYTE(0x0000),
	HIGH_BYTE(0x0000),
	0x00,	/* iChannelNames */
	0x00,	/* iTerminal */

	/* Audio Control Output Terminal Descriptor */
	UAC_DT_OUTPUT_TERMINAL_SIZE,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_OUTPUT_TERMINAL,	/* bDescriptorSubtype */
	0x02,	/* bTerminalID */
	LOW_BYTE(UAC_TERMINAL_STREAMING),	/* wTerminalType:(USB streaming) */
	HIGH_BYTE(UAC_TERMINAL_STREAMING),
	0x00,	/* bAssocTerminal */
	0x01,	/* bSourceID */
	0x00,	/* iTerminal */

	/* Interface Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,	/* bDescriptorType */
	0x01,	/* bInterfaceNumber */
	0x00,	/* bAlternateSetting */
	0x00,	/* bNumEndpoints */
	/* bInterfaceClass:Audio Interface Class */
	USB_CLASS_AUDIO,
	/* Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,	/* bInterfaceProtocol */
	0x00,	/* iInterface */

	/* Interface Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,	/* bDescriptorType */
	0x01,	/* bInterfaceNumber */
	0x01,	/* bAlternateSetting */
	0x01,	/* bNumEndpoints */
	/* bInterfaceClass:Audio Interface Class */
	USB_CLASS_AUDIO,
	/* Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,	/* bInterfaceProtocol */
	0x00,	/* iInterface */

	/* Audio Streaming Class Specific Interface Descriptor */
	UAC_DT_AS_HEADER_SIZE,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_AS_GENERAL,	/* bDescriptorSubtype */
	0x02,	/* bTerminalLink */
	0x01,	/* bDelay */
	LOW_BYTE(UAC_FORMAT_TYPE_I_PCM),	/* wFormatTag:(PCM) */
	HIGH_BYTE(UAC_FORMAT_TYPE_I_PCM),

	/* Audio Streaming Format Type Descriptor */
	0x0B,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_FORMAT_TYPE,	/* bDescriptorSubtype */
	UAC_FORMAT_TYPE_I,	/* bFormatType */
	CONFIG_USB_AUDIO_SOURCE_UPLOAD_CHANNEL_NUM,	/* bNrChannels */
	SUB_FRAME_SIZE,	/* bSubframeSize */
	RESOLUTION,	/* bBitResolution */
	0x01,		/* bSamFreqType */
	SAM_LOW_BYTE(CONFIG_USB_AUDIO_SOURCE_SAMPLE_RATE),	/*tSamFreq[n]*/
	SAM_MIDDLE_BYTE(CONFIG_USB_AUDIO_SOURCE_SAMPLE_RATE),
	SAM_HIGH_BYTE(CONFIG_USB_AUDIO_SOURCE_SAMPLE_RATE),

	/* Standard Endpoint Descriptor */
	0x07,	/* bLength */
	USB_ENDPOINT_DESC,	/* bDescriptorType */
	CONFIG_USB_AUDIO_SOURCE_IN_EP_ADDR,	/* Direction: IN - EndpointID: n */
	0x01,	/* Synchronization Type = Synchronous */
	LOW_BYTE(MAX_UPLOAD_PACKET),	/* wMaxPacketSize */
	HIGH_BYTE(MAX_UPLOAD_PACKET),
	CONFIG_USB_AUDIO_SOURCE_IN_EP_FS_INTERVAL,	/* bInterval */

	/* Audio Streaming Class Specific Audio Data Endpoint Descriptor */
	UAC_ISO_ENDPOINT_DESC_SIZE,	/* bLength */
	CS_ENDPOINT,	/* bDescriptorType */
	UAC_EP_GENERAL,	/* bDescriptorSubtype */
	0x00,	/* bmAttributes:No sampling frequency control, no pitch control, no packet padding.*/
	0x00,	/* bLockDelayUnits */
	LOW_BYTE(0x0001),	/* wLockDelay */
	HIGH_BYTE(0x0001),
};

static const u8_t usb_audio_source_hs_descriptor[] = {
	/* Device Descriptor */
	USB_DEVICE_DESC_SIZE,	/* bLength */
	USB_DEVICE_DESC,	/* bDescriptorType */
	LOW_BYTE(USB_2_0),	/* bcdUSB */
	HIGH_BYTE(USB_2_0),
	0x00,	/* bDeviceClass */
	0x00,	/* bDeviceSubClass */
	0x00,	/* bDeviceProtocol */
	MAX_PACKET_SIZE0,	/* bMaxPacketSize0 = (64) Bytes */
	LOW_BYTE(CONFIG_USB_AUDIO_SOURCE_VID),	/* idVendor VID */
	HIGH_BYTE(CONFIG_USB_AUDIO_SOURCE_VID),
	LOW_BYTE(CONFIG_USB_AUDIO_SOURCE_PID),	/* idProduct PID */
	HIGH_BYTE(CONFIG_USB_AUDIO_SOURCE_PID),
	LOW_BYTE(BCDDEVICE_RELNUM),	/* bcdDevice */
	HIGH_BYTE(BCDDEVICE_RELNUM),
	0x01,	/* iManufacturer */
	0x02,	/* iProduct */
	0x03,	/* iSerialNumber */
	0x01,	/* bNumConfigurations */

	/* Configuration Descriptor */
	USB_CONFIGURATION_DESC_SIZE,	/* bLength */
	USB_CONFIGURATION_DESC,	/* bDescriptorType */
	LOW_BYTE(0x0062),	/* wTotalLength */
	HIGH_BYTE(0x0062),
	0x02,	/* bNumInterfaces */
	0x01,	/* bConfigurationValue */
	0x00,	/* iConfiguration */
	USB_CONFIGURATION_ATTRIBUTES,	/* bmAttributes */
	MAX_LOW_POWER,	/* MaxPower = 100 mA */

	/* Interface Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,	/* bDescriptorType */
	0x00,	/* bInterfaceNumber */
	0x00,	/* bAlternateSetting */
	0x00,	/* bNumEndpoints */
	USB_CLASS_AUDIO,	/* bInterfaceClass:  Audio Interface Class */
	USB_SUBCLASS_AUDIOCONTROL,	/* bInterface SubClass */
	0x00,	/* bInterfaceProtocol */
	0x00,	/* iInterface */

	/* Class-Specific AC Interface Descriptor */
	/* 1. Audio Control Interface Header Descriptor */
	0x09,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_HEADER,	/* bDescriptorSubtype */
	LOW_BYTE(0x0100),	/* bcdADC */
	HIGH_BYTE(0x0100),
	/* wTotalLength[30/0x1E] = len(header_desc[9] + ITD[12] + OTD[9]) */
	LOW_BYTE(0x001E),
	HIGH_BYTE(0x001E),
	0x01,	/* bInCollection */
	0x01,	/* baInterfaceNr[1] */

	/* Audio Control Input Terminal Descriptor */
	UAC_DT_INPUT_TERMINAL_SIZE,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_INPUT_TERMINAL,	/* bDescriptorSubtype */
	0x01,	/* bTerminalID */
	LOW_BYTE(UAC_INPUT_TERMINAL_MICROPHONE),	/* wTerminalType:(Microphone) */
	HIGH_BYTE(UAC_INPUT_TERMINAL_MICROPHONE),
	0x00,	/* bAssocTerminal */
	0x01,	/* One channe */
	/* wChannelConfig: Single channel does not set bit mapping */
	LOW_BYTE(0x0000),
	HIGH_BYTE(0x0000),
	0x00,	/* iChannelNames */
	0x00,	/* iTerminal */

	/* Audio Control Output Terminal Descriptor */
	UAC_DT_OUTPUT_TERMINAL_SIZE,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_OUTPUT_TERMINAL,	/* bDescriptorSubtype */
	0x02,	/* bTerminalID */
	LOW_BYTE(UAC_TERMINAL_STREAMING),	/* wTerminalType:(USB streaming) */
	HIGH_BYTE(UAC_TERMINAL_STREAMING),
	0x00,	/* bAssocTerminal */
	0x01,	/* bSourceID */
	0x00,	/* iTerminal */

	/* Interface Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,	/* bDescriptorType */
	0x01,	/* bInterfaceNumber */
	0x00,	/* bAlternateSetting */
	0x00,	/* bNumEndpoints */
	/* bInterfaceClass:Audio Interface Class */
	USB_CLASS_AUDIO,
	/* Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,	/* bInterfaceProtocol */
	0x00,	/* iInterface */

	/* Interface Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,	/* bDescriptorType */
	0x01,	/* bInterfaceNumber */
	0x01,	/* bAlternateSetting */
	0x01,	/* bNumEndpoints */
	/* bInterfaceClass:Audio Interface Class */
	USB_CLASS_AUDIO,
	/* Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,	/* bInterfaceProtocol */
	0x00,	/* iInterface */

	/* Audio Streaming Class Specific Interface Descriptor */
	UAC_DT_AS_HEADER_SIZE,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_AS_GENERAL,	/* bDescriptorSubtype */
	0x02,	/* bTerminalLink */
	0x01,	/* bDelay */
	LOW_BYTE(UAC_FORMAT_TYPE_I_PCM),	/* wFormatTag:(PCM) */
	HIGH_BYTE(UAC_FORMAT_TYPE_I_PCM),

	/* Audio Streaming Format Type Descriptor */
	0x0B,	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_FORMAT_TYPE,	/* bDescriptorSubtype */
	UAC_FORMAT_TYPE_I,	/* bFormatType */
	CONFIG_USB_AUDIO_SOURCE_UPLOAD_CHANNEL_NUM,	/* bNrChannels */
	SUB_FRAME_SIZE,	/* bSubframeSize */
	RESOLUTION,	/* bBitResolution */
	0x01,		/* bSamFreqType */
	SAM_LOW_BYTE(CONFIG_USB_AUDIO_SOURCE_SAMPLE_RATE),	/*tSamFreq[n]*/
	SAM_MIDDLE_BYTE(CONFIG_USB_AUDIO_SOURCE_SAMPLE_RATE),
	SAM_HIGH_BYTE(CONFIG_USB_AUDIO_SOURCE_SAMPLE_RATE),

	/* Standard Endpoint Descriptor */
	0x07,	/* bLength */
	USB_ENDPOINT_DESC,	/* bDescriptorType */
	CONFIG_USB_AUDIO_SOURCE_IN_EP_ADDR,	/* Direction: IN - EndpointID: n */
	0x01,	/* Synchronization Type = Synchronous */
	LOW_BYTE(MAX_UPLOAD_PACKET),	/* wMaxPacketSize */
	HIGH_BYTE(MAX_UPLOAD_PACKET),
	CONFIG_USB_AUDIO_SOURCE_IN_EP_HS_INTERVAL,	/* bInterval */

	/* Audio Streaming Class Specific Audio Data Endpoint Descriptor */
	UAC_ISO_ENDPOINT_DESC_SIZE,	/* bLength */
	CS_ENDPOINT,	/* bDescriptorType */
	UAC_EP_GENERAL,	/* bDescriptorSubtype */
	0x00,	/* bmAttributes:No sampling frequency control, no pitch control, no packet padding.*/
	0x00,	/* bLockDelayUnits */
	LOW_BYTE(0x0001),	/* wLockDelay */
	HIGH_BYTE(0x0001),
};

#endif	/* __USB_AUDIO_SOURCE_DESC_H__ */
