/*
 * USB Audio Class header
 *
 * Copyright (c) 2018 Actions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __USB_AUDIO_H__
#define __USB_AUDIO_H__

#include <usb/usb_device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* bInterfaceProtocol values to denote the version of the standard used */
#define UAC_VERSION_1			0x00
#define UAC_VERSION_2			0x20

/* A.2 Audio Interface Subclass Codes */
#define USB_SUBCLASS_AUDIOCONTROL	0x01
#define USB_SUBCLASS_AUDIOSTREAMING	0x02
#define USB_SUBCLASS_MIDISTREAMING	0x03

/* A.5 Audio Class-Specific AC Interface Descriptor Subtypes */
#define UAC_HEADER			0x01
#define UAC_INPUT_TERMINAL		0x02
#define UAC_OUTPUT_TERMINAL		0x03
#define UAC_MIXER_UNIT			0x04
#define UAC_SELECTOR_UNIT		0x05
#define UAC_FEATURE_UNIT		0x06
#define UAC1_PROCESSING_UNIT		0x07
#define UAC1_EXTENSION_UNIT		0x08

/* A.6 Audio Class-Specific AS Interface Descriptor Subtypes */
#define UAC_AS_GENERAL			0x01
#define UAC_FORMAT_TYPE			0x02
#define UAC_FORMAT_SPECIFIC		0x03

/* A.7 Processing Unit Process Types */
#define UAC_PROCESS_UNDEFINED		0x00
#define UAC_PROCESS_UP_DOWNMIX		0x01
#define UAC_PROCESS_DOLBY_PROLOGIC	0x02
#define UAC_PROCESS_STEREO_EXTENDER	0x03
#define UAC_PROCESS_REVERB		0x04
#define UAC_PROCESS_CHORUS		0x05
#define UAC_PROCESS_DYN_RANGE_COMP	0x06

/* A.8 Audio Class-Specific Endpoint Descriptor Subtypes */
#define UAC_EP_GENERAL			0x01

/* A.9 Audio Class-Specific Request Codes */
#define UAC_SET_			0x00
#define UAC_GET_			0x80

#define UAC__CUR			0x1
#define UAC__MIN			0x2
#define UAC__MAX			0x3
#define UAC__RES			0x4
#define UAC__MEM			0x5

#define UAC_SET_CUR			(UAC_SET_ | UAC__CUR)
#define UAC_GET_CUR			(UAC_GET_ | UAC__CUR)
#define UAC_SET_MIN			(UAC_SET_ | UAC__MIN)
#define UAC_GET_MIN			(UAC_GET_ | UAC__MIN)
#define UAC_SET_MAX			(UAC_SET_ | UAC__MAX)
#define UAC_GET_MAX			(UAC_GET_ | UAC__MAX)
#define UAC_SET_RES			(UAC_SET_ | UAC__RES)
#define UAC_GET_RES			(UAC_GET_ | UAC__RES)
#define UAC_SET_MEM			(UAC_SET_ | UAC__MEM)
#define UAC_GET_MEM			(UAC_GET_ | UAC__MEM)

#define UAC_GET_STAT			0xff

/* A.10 Control Selector Codes */

/* A.10.1 Terminal Control Selectors */
#define UAC_TERM_COPY_PROTECT		0x01

/* A.10.2 Feature Unit Control Selectors */
#define UAC_FU_MUTE			0x01
#define UAC_FU_VOLUME			0x02
#define UAC_FU_BASS			0x03
#define UAC_FU_MID			0x04
#define UAC_FU_TREBLE			0x05
#define UAC_FU_GRAPHIC_EQUALIZER	0x06
#define UAC_FU_AUTOMATIC_GAIN		0x07
#define UAC_FU_DELAY			0x08
#define UAC_FU_BASS_BOOST		0x09
#define UAC_FU_LOUDNESS			0x0a

#define UAC_CONTROL_BIT(CS)	(1 << ((CS) - 1))

/* A.10.3.1 Up/Down-mix Processing Unit Controls Selectors */
#define UAC_UD_ENABLE			0x01
#define UAC_UD_MODE_SELECT		0x02

/* A.10.3.2 Dolby Prologic (tm) Processing Unit Controls Selectors */
#define UAC_DP_ENABLE			0x01
#define UAC_DP_MODE_SELECT		0x02

/* A.10.3.3 3D Stereo Extender Processing Unit Control Selectors */
#define UAC_3D_ENABLE			0x01
#define UAC_3D_SPACE			0x02

/* A.10.3.4 Reverberation Processing Unit Control Selectors */
#define UAC_REVERB_ENABLE		0x01
#define UAC_REVERB_LEVEL		0x02
#define UAC_REVERB_TIME			0x03
#define UAC_REVERB_FEEDBACK		0x04

/* A.10.3.5 Chorus Processing Unit Control Selectors */
#define UAC_CHORUS_ENABLE		0x01
#define UAC_CHORUS_LEVEL		0x02
#define UAC_CHORUS_RATE			0x03
#define UAC_CHORUS_DEPTH		0x04

/* A.10.3.6 Dynamic Range Compressor Unit Control Selectors */
#define UAC_DCR_ENABLE			0x01
#define UAC_DCR_RATE			0x02
#define UAC_DCR_MAXAMPL			0x03
#define UAC_DCR_THRESHOLD		0x04
#define UAC_DCR_ATTACK_TIME		0x05
#define UAC_DCR_RELEASE_TIME		0x06

/* A.10.4 Extension Unit Control Selectors */
#define UAC_XU_ENABLE			0x01

/* MIDI - A.1 MS Class-Specific Interface Descriptor Subtypes */
#define UAC_MS_HEADER			0x01
#define UAC_MIDI_IN_JACK		0x02
#define UAC_MIDI_OUT_JACK		0x03

/* MIDI - A.1 MS Class-Specific Endpoint Descriptor Subtypes */
#define UAC_MS_GENERAL			0x01

/* Terminals - 2.1 USB Terminal Types */
#define UAC_TERMINAL_UNDEFINED		0x100
#define UAC_TERMINAL_STREAMING		0x101
#define UAC_TERMINAL_VENDOR_SPEC	0x1FF

/* feature unit control selectors */
#define FU_CONTROL_UNDEFINED           0x00
#define MUTE_CONTROL                   0x01
#define VOLUME_CONTROL                 0x02
#define BASS_CONTROL                   0x03
#define MID_CONTROL                    0x04
#define TREBLE_CONTROL                 0x05
#define GRAPHIC_EQUALIZER_CONTROL      0x06
#define AUTOMATIC_GAIN_CONTROL         0x07
#define DELAY_CONTROL                  0x08
#define BASS_BOOST_CONTROL             0x09
#define LOUDNESS_CONTROL               0x0A

/* channel number */
#define MAIN_CHANNEL_NUMBER0           0x00
#define MAIN_CHANNEL_NUMBER1           0x01
#define MAIN_CHANNEL_NUMBER2           0x02

/* volume configuration of max, min, resolution */
#define MAXIMUM_VOLUME                 0x0000
#define MINIMUM_VOLUME                 0xC3D8
#define RESOTION_VOLUME                0x009A

#define MUTE_LENGTH                    0x01
#define VOLUME_LENGTH                  0x02
#define SAMPLE_FREQUENCY_LENGTH        0x03


/* Terminal Control Selectors */
/* 4.3.2  Class-Specific AC Interface Descriptor */
struct uac1_ac_header_descriptor {
	uint8_t  bLength;			/* 8 + n */
	uint8_t  bDescriptorType;		/* USB_DT_CS_INTERFACE */
	uint8_t  bDescriptorSubtype;	/* UAC_MS_HEADER */
	uint16_t bcdADC;			/* 0x0100 */
	uint16_t wTotalLength;		/* includes Unit and Terminal desc. */
	uint8_t  bInCollection;		/* n */
	uint8_t  baInterfaceNr[];		/* [n] */
} __packed;

#define UAC_DT_AC_HEADER_SIZE(n)	(8 + (n))

/* As above, but more useful for defining your own descriptors: */
#define DECLARE_UAC_AC_HEADER_DESCRIPTOR(n)			\
struct uac1_ac_header_descriptor_##n {			\
	uint8_t  bLength;						\
	uint8_t  bDescriptorType;					\
	uint8_t  bDescriptorSubtype;				\
	uint16_t bcdADC;						\
	uint16_t wTotalLength;					\
	uint8_t  bInCollection;					\
	uint8_t  baInterfaceNr[n];					\
} __packed

/* 4.3.2.1 Input Terminal Descriptor */
struct uac_input_terminal_descriptor {
	uint8_t  bLength;			/* in bytes: 12 */
	uint8_t  bDescriptorType;		/* CS_INTERFACE descriptor type */
	uint8_t  bDescriptorSubtype;	/* INPUT_TERMINAL descriptor subtype */
	uint8_t  bTerminalID;		/* Constant uniquely terminal ID */
	uint16_t wTerminalType;		/* USB Audio Terminal Types */
	uint8_t  bAssocTerminal;		/* ID of the Output Terminal associated */
	uint8_t  bNrChannels;		/* Number of logical output channels */
	uint16_t wChannelConfig;
	uint8_t  iChannelNames;
	uint8_t  iTerminal;
} __packed;

#define UAC_DT_INPUT_TERMINAL_SIZE			12

/* Terminals - 2.2 Input Terminal Types */
#define UAC_INPUT_TERMINAL_UNDEFINED			0x200
#define UAC_INPUT_TERMINAL_MICROPHONE			0x201
#define UAC_INPUT_TERMINAL_DESKTOP_MICROPHONE		0x202
#define UAC_INPUT_TERMINAL_PERSONAL_MICROPHONE		0x203
#define UAC_INPUT_TERMINAL_OMNI_DIR_MICROPHONE		0x204
#define UAC_INPUT_TERMINAL_MICROPHONE_ARRAY		0x205
#define UAC_INPUT_TERMINAL_PROC_MICROPHONE_ARRAY	0x206

/* Terminals - control selectors */
#define UAC_TERMINAL_CS_COPY_PROTECT_CONTROL		0x01

/* 4.3.2.2 Output Terminal Descriptor */
struct uac1_output_terminal_descriptor {
	uint8_t  bLength;			/* in bytes: 9 */
	uint8_t  bDescriptorType;		/* CS_INTERFACE descriptor type */
	uint8_t  bDescriptorSubtype;	/* OUTPUT_TERMINAL descriptor subtype */
	uint8_t  bTerminalID;		/* Constant uniquely terminal ID */
	uint16_t wTerminalType;		/* USB Audio Terminal Types */
	uint8_t  bAssocTerminal;		/* ID of the Input Terminal associated */
	uint8_t  bSourceID;		/* ID of the connected Unit or Terminal*/
	uint8_t  iTerminal;
} __packed;

struct uac_feature_unit_descriptor_1 {
	uint8_t bLength; 			/* Size of this descriptor, in bytes:7+(ch+1)*n */
	uint8_t bDescriptorType; 		/* CS_INTERFACE descriptor type */
	uint8_t bDescriptorSubtype; 	/* FEATURE_UNIT descriptor subtype */
	uint8_t bUnitID; 			/*Constant uniquely identifying the Unit within the audio function.This value is used in all requests to address this Unit.*/
	uint8_t bSourceID; 		/* ID of the Unit or Terminal to which this Feature Unit is connected*/
	uint8_t bControlSize; 		/* Size in bytes of an element of the bmaControls() array: n */
	uint8_t bmaControls0; 		/*A bit set to 1 indicates that the mentioned Control is supported for master channel 0
	D0: Mute D1: Volume D2: Bass D3: Mid D4: Treble D5: Graphic Equalizer
	D6: Automatic Gain D7: Delay D8: Bass Boost D9: Loudness D10..(n*8-1): Reserved */
	uint8_t bmaControls1; 		/*A bit set to 1 indicates
	that the mentioned Control is supported for logical channel 1*/
	uint8_t bmaControls2; 		/*A bit set to 1 indicates
	that the mentioned Control is supported for logical channel 2*/
	uint8_t iFeature; 			/* Index of a string descriptor, describing this Feature Unit */
} __packed; 				/* Table 4-7: Feature Unit Descriptor */

struct uac_feature_unit_descriptor_2 {
	uint8_t bLength; 			/* Size of this descriptor, in bytes:7+(ch+1)*n */
	uint8_t bDescriptorType; 		/* CS_INTERFACE descriptor type */
	uint8_t bDescriptorSubtype; 	/* FEATURE_UNIT descriptor subtype */
	uint8_t bUnitID; 			/*Constant uniquely identifying the Unit within the audio function.
	This value is used in all requests to address this Unit.*/
	uint8_t bSourceID; 		/* ID of the Unit or Terminal to which this Feature Unit is connected*/
	uint8_t bControlSize; 		/* Size in bytes of an element of the bmaControls() array: n */
	uint8_t bmaControls0; 		/*A bit set to 1 indicates that the mentioned Control is supported for master channel 0
	D0: Mute D1: Volume D2: Bass D3: Mid D4: Treble D5: Graphic Equalizer
	D6: Automatic Gain D7: Delay D8: Bass Boost D9: Loudness D10..(n*8-1): Reserved */
	uint8_t bmaControls1; 		/*A bit set to 1 indicates
	that the mentioned Control is supported for logical channel 1*/
	uint8_t iFeature; 			/* Index of a string descriptor, describing this Feature Unit */
} __packed; 				/* Table 4-7: Feature Unit Descriptor */



#define UAC_DT_OUTPUT_TERMINAL_SIZE			9

/* Terminals - 2.3 Output Terminal Types */
#define UAC_OUTPUT_TERMINAL_UNDEFINED			0x300
#define UAC_OUTPUT_TERMINAL_SPEAKER			0x301
#define UAC_OUTPUT_TERMINAL_HEADPHONES			0x302
#define UAC_OUTPUT_TERMINAL_HEAD_MOUNTED_DISPLAY_AUDIO	0x303
#define UAC_OUTPUT_TERMINAL_DESKTOP_SPEAKER		0x304
#define UAC_OUTPUT_TERMINAL_ROOM_SPEAKER		0x305
#define UAC_OUTPUT_TERMINAL_COMMUNICATION_SPEAKER	0x306
#define UAC_OUTPUT_TERMINAL_LOW_FREQ_EFFECTS_SPEAKER	0x307

#define UAC_DT_FEATURE_UNIT_SIZE(ch, Size)		(7 + ((ch) + 1) * Size)

enum sync_info_type {
	USOUND_SYNC_HOST_VOL_TYPE,
	USOUND_SYNC_DEVICE_VOL_TYPE,
	USOUND_OPEN_UPLOAD_CHANNEL,
	USOUND_CLOSE_UPLOAD_CHANNEL,
	USOUND_STREAM_START,
	USOUND_STREAM_STOP,
	USOUND_SYNC_HOST_PAUSE,
	USOUND_SYNC_HOST_RESUME,
	USOUND_SYNC_HOST_MUTE,
	USOUND_SYNC_HOST_UNMUTE,
	USOUND_SYNC_DEVICE_VOL_INC,
	USOUND_SYNC_DEVICE_VOL_DEC,
};

/* As above, but more useful for defining your own descriptors: */
#define DECLARE_UAC_FEATURE_UNIT_DESCRIPTOR(ch)			\
struct uac_feature_unit_descriptor_##ch {			\
	uint8_t  bLength;						\
	uint8_t  bDescriptorType;					\
	uint8_t  bDescriptorSubtype;				\
	uint8_t  bUnitID;						\
	uint8_t  bSourceID;					\
	uint8_t  bControlSize;					\
	uint16_t bmaControls[ch + 1];				\
	uint8_t  iFeature;						\
} __packed

/* 4.3.2.3 Mixer Unit Descriptor */
struct uac_mixer_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint8_t bNrInPins;
	uint8_t baSourceID[];
} __packed;

static inline uint8_t uac_mixer_unit_bNrChannels(struct uac_mixer_unit_descriptor *desc)
{
	return desc->baSourceID[desc->bNrInPins];
}

static inline uint32_t uac_mixer_unit_wChannelConfig(struct uac_mixer_unit_descriptor *desc,
						  int protocol)
{
	if (protocol == UAC_VERSION_1)
		return (desc->baSourceID[desc->bNrInPins + 2] << 8) |
			desc->baSourceID[desc->bNrInPins + 1];
	else
		return  (desc->baSourceID[desc->bNrInPins + 4] << 24) |
			(desc->baSourceID[desc->bNrInPins + 3] << 16) |
			(desc->baSourceID[desc->bNrInPins + 2] << 8)  |
			(desc->baSourceID[desc->bNrInPins + 1]);
}

static inline uint8_t uac_mixer_unit_iChannelNames(struct uac_mixer_unit_descriptor *desc,
						int protocol)
{
	return (protocol == UAC_VERSION_1) ?
		desc->baSourceID[desc->bNrInPins + 3] :
		desc->baSourceID[desc->bNrInPins + 5];
}

static inline uint8_t *uac_mixer_unit_bmControls(struct uac_mixer_unit_descriptor *desc,
					      int protocol)
{
	return (protocol == UAC_VERSION_1) ?
		&desc->baSourceID[desc->bNrInPins + 4] :
		&desc->baSourceID[desc->bNrInPins + 6];
}

static inline uint8_t uac_mixer_unit_iMixer(struct uac_mixer_unit_descriptor *desc)
{
	uint8_t *raw = (uint8_t *) desc;
	return raw[desc->bLength - 1];
}

/* 4.3.2.4 Selector Unit Descriptor */
struct uac_selector_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUintID;
	uint8_t bNrInPins;
	uint8_t baSourceID;
  #ifdef AUDIO_SOURCE_HAS_FEATURE_UNIT
	uint8_t bSelector; /* index of a string descriptor,describing the select unit */
  #endif
} __packed;

static inline uint8_t uac_selector_unit_iSelector(struct uac_selector_unit_descriptor *desc)
{
	uint8_t *raw = (uint8_t *) desc;
	return raw[desc->bLength - 1];
}

/* 4.3.2.5 Feature Unit Descriptor */
struct uac_feature_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint8_t bSourceID;
	uint8_t bControlSize;
	uint8_t bmaControls[0]; /* variable length */
} __packed;

static inline uint8_t uac_feature_unit_iFeature(struct uac_feature_unit_descriptor *desc)
{
	uint8_t *raw = (uint8_t *) desc;
	return raw[desc->bLength - 1];
}

/* 4.3.2.6 Processing Unit Descriptors */
struct uac_processing_unit_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bUnitID;
	uint16_t wProcessType;
	uint8_t bNrInPins;
	uint8_t baSourceID[];
} __packed;

static inline uint8_t uac_processing_unit_bNrChannels(struct uac_processing_unit_descriptor *desc)
{
	return desc->baSourceID[desc->bNrInPins];
}

static inline uint32_t uac_processing_unit_wChannelConfig(struct uac_processing_unit_descriptor *desc,
						       int protocol)
{
	if (protocol == UAC_VERSION_1)
		return (desc->baSourceID[desc->bNrInPins + 2] << 8) |
			desc->baSourceID[desc->bNrInPins + 1];
	else
		return  (desc->baSourceID[desc->bNrInPins + 4] << 24) |
			(desc->baSourceID[desc->bNrInPins + 3] << 16) |
			(desc->baSourceID[desc->bNrInPins + 2] << 8)  |
			(desc->baSourceID[desc->bNrInPins + 1]);
}

static inline uint8_t uac_processing_unit_iChannelNames(struct uac_processing_unit_descriptor *desc,
						     int protocol)
{
	return (protocol == UAC_VERSION_1) ?
		desc->baSourceID[desc->bNrInPins + 3] :
		desc->baSourceID[desc->bNrInPins + 5];
}

static inline uint8_t uac_processing_unit_bControlSize(struct uac_processing_unit_descriptor *desc,
						    int protocol)
{
	return (protocol == UAC_VERSION_1) ?
		desc->baSourceID[desc->bNrInPins + 4] :
		desc->baSourceID[desc->bNrInPins + 6];
}

static inline uint8_t *uac_processing_unit_bmControls(struct uac_processing_unit_descriptor *desc,
						   int protocol)
{
	return (protocol == UAC_VERSION_1) ?
		&desc->baSourceID[desc->bNrInPins + 5] :
		&desc->baSourceID[desc->bNrInPins + 7];
}

static inline uint8_t uac_processing_unit_iProcessing(struct uac_processing_unit_descriptor *desc,
						   int protocol)
{
	uint8_t control_size = uac_processing_unit_bControlSize(desc, protocol);

	return *(uac_processing_unit_bmControls(desc, protocol)
			+ control_size);
}

static inline uint8_t *uac_processing_unit_specific(struct uac_processing_unit_descriptor *desc,
						 int protocol)
{
	uint8_t control_size = uac_processing_unit_bControlSize(desc, protocol);

	return uac_processing_unit_bmControls(desc, protocol)
			+ control_size + 1;
}

/* 4.5.2 Class-Specific AS Interface Descriptor */
struct uac1_as_header_descriptor {
	uint8_t  bLength;			/* in bytes: 7 */
	uint8_t  bDescriptorType;		/* USB_DT_CS_INTERFACE */
	uint8_t  bDescriptorSubtype;	/* AS_GENERAL */
	uint8_t  bTerminalLink;		/* Terminal ID of connected Terminal */
	uint8_t  bDelay;			/* Delay introduced by the data path */
	uint16_t wFormatTag;		/* The Audio Data Format */
} __packed;

#define UAC_DT_AS_HEADER_SIZE		7

/* Formats - A.1.1 Audio Data Format Type I Codes */
#define UAC_FORMAT_TYPE_I_UNDEFINED	0x0
#define UAC_FORMAT_TYPE_I_PCM		0x1
#define UAC_FORMAT_TYPE_I_PCM8		0x2
#define UAC_FORMAT_TYPE_I_IEEE_FLOAT	0x3
#define UAC_FORMAT_TYPE_I_ALAW		0x4
#define UAC_FORMAT_TYPE_I_MULAW		0x5

struct uac_format_type_i_continuous_descriptor {
	uint8_t  bLength;			/* in bytes: 8 + (ns * 3) */
	uint8_t  bDescriptorType;		/* USB_DT_CS_INTERFACE */
	uint8_t  bDescriptorSubtype;	/* FORMAT_TYPE */
	uint8_t  bFormatType;		/* FORMAT_TYPE_1 */
	uint8_t  bNrChannels;		/* physical channels in the stream */
	uint8_t  bSubframeSize;
	uint8_t  bBitResolution;
	uint8_t  bSamFreqType;
	uint8_t  tLowerSamFreq[3];
	uint8_t  tUpperSamFreq[3];
} __packed;

#define UAC_FORMAT_TYPE_I_CONTINUOUS_DESC_SIZE	14

struct uac_format_type_i_discrete_descriptor {
	uint8_t  bLength;			/* in bytes: 8 + (ns * 3) */
	uint8_t  bDescriptorType;		/* USB_DT_CS_INTERFACE */
	uint8_t  bDescriptorSubtype;	/* FORMAT_TYPE */
	uint8_t  bFormatType;		/* FORMAT_TYPE_1 */
	uint8_t  bNrChannels;		/* physical channels in the stream */
	uint8_t  bSubframeSize;
	uint8_t  bBitResolution;
	uint8_t  bSamFreqType;
	uint8_t  tSamFreq[][3];
} __packed;

#define DECLARE_UAC_FORMAT_TYPE_I_DISCRETE_DESC(n)		\
struct uac_format_type_i_discrete_descriptor_##n {		\
	uint8_t  bLength;						\
	uint8_t  bDescriptorType;					\
	uint8_t  bDescriptorSubtype;				\
	uint8_t  bFormatType;					\
	uint8_t  bNrChannels;					\
	uint8_t  bSubframeSize;					\
	uint8_t  bBitResolution;					\
	uint8_t  bSamFreqType;					\
	uint8_t  tSamFreq[n][3];					\
} __packed

#define DECLARE_UAC_FORMAT_TYPE_O_DISCRETE_DESC(n)		\
struct uac_format_type_o_discrete_descriptor_##n {		\
	uint8_t  bLength;						\
	uint8_t  bDescriptorType;					\
	uint8_t  bDescriptorSubtype;				\
	uint8_t  bFormatType;					\
	uint8_t  bNrChannels;					\
	uint8_t  bSubframeSize;					\
	uint8_t  bBitResolution;					\
	uint8_t  bSamFreqType;					\
	uint8_t  tSamFreq[n][3];					\
} __packed

#define UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(n)	(8 + (n * 3))
#define UAC_FORMAT_TYPE_O_DISCRETE_DESC_SIZE(n)	(8 + (n * 3))

struct uac_format_type_i_ext_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFormatType;
	uint8_t bSubslotSize;
	uint8_t bBitResolution;
	uint8_t bHeaderLength;
	uint8_t bControlSize;
	uint8_t bSideBandProtocol;
} __packed;

/* Formats - Audio Data Format Type I Codes */

#define UAC_FORMAT_TYPE_II_MPEG	0x1001
#define UAC_FORMAT_TYPE_II_AC3	0x1002

struct uac_format_type_ii_discrete_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFormatType;
	uint16_t wMaxBitRate;
	uint16_t wSamplesPerFrame;
	uint8_t bSamFreqType;
	uint8_t tSamFreq[][3];
} __packed;

struct uac_format_type_ii_ext_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;
	uint8_t bFormatType;
	uint16_t wMaxBitRate;
	uint16_t wSamplesPerFrame;
	uint8_t bHeaderLength;
	uint8_t bSideBandProtocol;
} __packed;

/* type III */
#define UAC_FORMAT_TYPE_III_IEC1937_AC3			0x2001
#define UAC_FORMAT_TYPE_III_IEC1937_MPEG1_LAYER1	0x2002
#define UAC_FORMAT_TYPE_III_IEC1937_MPEG2_NOEXT		0x2003
#define UAC_FORMAT_TYPE_III_IEC1937_MPEG2_EXT		0x2004
#define UAC_FORMAT_TYPE_III_IEC1937_MPEG2_LAYER1_LS	0x2005
#define UAC_FORMAT_TYPE_III_IEC1937_MPEG2_LAYER23_LS	0x2006

/* Formats - A.2 Format Type Codes */
#define UAC_FORMAT_TYPE_UNDEFINED	0x0
#define UAC_FORMAT_TYPE_I		0x1
#define UAC_FORMAT_TYPE_II		0x2
#define UAC_FORMAT_TYPE_III		0x3
#define UAC_EXT_FORMAT_TYPE_I		0x81
#define UAC_EXT_FORMAT_TYPE_II		0x82
#define UAC_EXT_FORMAT_TYPE_III		0x83

struct uac_iso_endpoint_descriptor {
	uint8_t  bLength;			/* in bytes: 7 */
	uint8_t  bDescriptorType;		/* USB_DT_CS_ENDPOINT */
	uint8_t  bDescriptorSubtype;	/* EP_GENERAL */
	uint8_t  bmAttributes;
	uint8_t  bLockDelayUnits;
	uint16_t wLockDelay;
} __packed;

#define UAC_ISO_ENDPOINT_DESC_SIZE	7

#define UAC_EP_CS_ATTR_SAMPLE_RATE	0x01
#define UAC_EP_CS_ATTR_PITCH_CONTROL	0x02
#define UAC_EP_CS_ATTR_FILL_MAX		0x80

#define SPECIFIC_REQUEST_OUT           	0x21
#define SPECIFIC_REQUEST_IN            	0xA1

/* status word format (3.7.1.1) */
#define UAC1_STATUS_TYPE_ORIG_MASK		0x0f
#define UAC1_STATUS_TYPE_ORIG_AUDIO_CONTROL_IF	0x0
#define UAC1_STATUS_TYPE_ORIG_AUDIO_STREAM_IF	0x1
#define UAC1_STATUS_TYPE_ORIG_AUDIO_STREAM_EP	0x2

#define UAC1_STATUS_TYPE_IRQ_PENDING		(1 << 7)
#define UAC1_STATUS_TYPE_MEM_CHANGED		(1 << 6)

struct uac1_status_word {
	uint8_t bStatusType;
	uint8_t bOriginator;
} __packed;

/* Macro definition aboult USB Audio */
#define USB_ENDPOINT_SYNCTYPE		0x0c
#define USB_ENDPOINT_SYNC_NONE		(0 << 2)
#define USB_ENDPOINT_SYNC_ASYNC		(1 << 2)
#define USB_ENDPOINT_SYNC_ADAPTIVE	(2 << 2)
#define USB_ENDPOINT_SYNC_SYNC		(3 << 2)
#define USB_CLASS_AUDIO	0x01

/**
 * Callback function for PM (suspend/resume).
 */
typedef void (*usb_audio_pm)(bool suspend);

/**
 * Callback function for audio sink (start/stop).
 */
typedef void (*usb_audio_start)(bool start);

/**
 * Callback function for audio volume sync.
 */
typedef void (*usb_audio_volume_sync)(uint8_t info_type, int chan_num, int *pstore_info);

/**
 * APIS of USB audio device(composite device: UAC + HID).
 */
/* initialize USB audio dev */
int usb_audio_device_init(void);

/* deinitialize USB audio dev */
int usb_audio_device_exit(void);

/* initialize USB audio composite device(UAC + HID) */
int usb_audio_composite_dev_init(void);

/* flush USB audio device in endpoint FIFO */
int usb_audio_source_inep_flush(void);

/* flush USB audio device out endpoint FIFO */
int usb_audio_sink_outep_flush(void);

/* register audio device start/stop callback */
void usb_audio_device_register_start_cb(usb_audio_start cb);

/* register endpoint data transfer complete callback */
void usb_audio_device_register_inter_in_ep_cb(usb_ep_callback cb);

void usb_audio_device_register_inter_out_ep_cb(usb_ep_callback cb);

/* register volume sync callback */
void usb_audio_device_register_volume_sync_cb(usb_audio_volume_sync cb);

/* USB audio device endpoint read/write function */
int usb_audio_device_ep_write(const uint8_t *data, uint32_t data_len, uint32_t *bytes_ret);

int usb_audio_device_ep_read(uint8_t *data, uint32_t data_len, uint32_t *bytes_ret);

/**
 * APIS of USB audio sink.
 */
int usb_audio_sink_init(struct device *dev);
int usb_audio_sink_deinit(void);
void usb_audio_sink_register_inter_out_ep_cb(usb_ep_callback cb);
void usb_audio_sink_register_volume_sync_cb(usb_audio_volume_sync cb);
int usb_audio_sink_ep_read(uint8_t *data, uint32_t data_len, uint32_t *bytes_ret);
int usb_audio_sink_ep_flush(void);

/**
 * APIS of USB audio source.
 */
int usb_audio_source_init(struct device *dev);
int usb_audio_source_deinit(void);
int usb_audio_source_ep_flush(void);
void usb_audio_source_register_start_cb(usb_audio_start cb);
void usb_audio_source_register_inter_in_ep_cb(usb_ep_callback cb);

#ifdef __cplusplus
}
#endif

#endif /* __USB_AUDIO_H__ */
