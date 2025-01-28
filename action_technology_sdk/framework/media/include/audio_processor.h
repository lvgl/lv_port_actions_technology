/*
 * Copyright (c) 2020, Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Audio processor interface
 */

#ifndef __AUDIO_PROCESSOR_H__
#define __AUDIO_PROCESSOR_H__

#include <zephyr.h>
#include <acts_ringbuf.h>

/**
 * @brief Audio processor APIs
 * @defgroup audio_processor_apis Audio Processor APIs
 * @ingroup media_system_apis
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#define MAKE_AUDIO_PROCESSOR_MODULE_ID(A, B, C, D) \
		(((uint32_t)(A) << 24) | ((uint32_t)(B) << 16) | ((uint32_t)(C) << 8) | (D))

#define MAKE_AUDIO_PROCESSOR_VERSION(major, minor, patchlevel) \
		(((uint32_t)((major) && 0xFF) << 24) | ((uint32_t)((minor) && 0xFF) << 16) | ((uint32_t)((patchlevel) && 0xFF) << 8))

#define AUDIO_PROCESSOR_VER_MAJOR(ver) ((ver >> 24) & 0xFF)
#define AUDIO_PROCESSOR_VER_MINOR(ver) ((ver >> 16) & 0xFF)
#define AUDIO_PROCESSOR_VER_PATCHLEVEL(ver) ((ver >> 8) & 0xFF)

#define MAX_AUDIO_CHANNELS (2)

typedef enum {
	BTCALL_PREPROCESSOR_MODULE_ID  = MAKE_AUDIO_PROCESSOR_MODULE_ID('B', 'T', 'C', 'R'),
	BTCALL_POSTPROCESSOR_MODULE_ID = MAKE_AUDIO_PROCESSOR_MODULE_ID('B', 'T', 'C', 'O'),

	MUSIC_POSTPROCESSOR_MODULE_ID  = MAKE_AUDIO_PROCESSOR_MODULE_ID('M', 'U', 'S', 'O'),
} audio_processor_module_id_e;

/** audio preprocessor init param */
typedef struct audio_preprocessor_init_param {
	/* audio_stream_type_e, see audio_system.h */
	uint8_t stream_type;   /* stream type of the media */
	uint8_t stream_effect; /* stream type of the effect applied on the media */

	uint8_t sample_rate; /* kHz */
	uint8_t sample_bits; /* bits per sample, 16/24/32 */
	uint8_t frac_bits;   /* fractional bits, 0 for integral samples */
	uint8_t channels;    /* channels of in/output pcms */
    uint8_t format; /* media_type_e */

	/* input pcm, separated channels */
	struct acts_ringbuf *inbufs[MAX_AUDIO_CHANNELS];
	/* input ref pcm as required, single channel in most cases */
	struct acts_ringbuf *refbuf;
	/* output pcm, separated channels */
	struct acts_ringbuf *outbufs[MAX_AUDIO_CHANNELS];

	/* processor specific params */
	void *ext_param;
} audio_preprocessor_init_param_t;

/** audio postprocessor init param */
typedef struct audio_postprocessor_init_param {
	/* audio_stream_type_e, see audio_system.h */
	uint8_t stream_type;   /* stream type of the media */
	uint8_t stream_effect; /* stream type of the effect applied on the media */

	uint8_t sample_rate; /* kHz */
	uint8_t sample_bits; /* bits per sample, 16/24/32 */
	uint8_t frac_bits;   /* fractional bits, 0 for integral samples */
	uint8_t channels;    /* channels of in/output pcm */
    uint8_t format; /* media_type_e */

	/* input pcm, separated channels */
	struct acts_ringbuf *inbufs[MAX_AUDIO_CHANNELS];
	/* output ref pcm as required, single channel in most cases */
	struct acts_ringbuf *refbuf;
	/* output pcm, interweaved channels */
	struct acts_ringbuf *outbuf;

	/* processor specific params */
	void *ext_param;
} audio_postprocessor_init_param_t;

struct audio_processor_module;

typedef struct audio_processor {
	const struct audio_processor_module *module;
	void *priv; /* private data */
} audio_processor_t;

/** audio pre/postprocessor module */
typedef struct audio_processor_module {
	/** Identifier of module */
	uint32_t id;
	/** Version of the implemented module */
	uint32_t version;
	/** Name of this module */
	const char *name;
	/** Module ops: initialize a processor, return 0 if succeed */
	int (*initialize)(audio_processor_t *handle, void *init_param);
	/** Module ops: destroy a processor, return 0 if succeed */
	int (*destroy)(audio_processor_t *handle);
	/** Module ops: run a processor, return number of sample (pair) successfully processed */
	int (*process)(audio_processor_t *handle);
	/** Module ops: flush a processor, return number of sample (pair) successfully flushed */
	int (*flush)(audio_processor_t *handle);
	/** Module ops: get parameters of a processor, return 0 if succeed */
	int (*get_parameter)(audio_processor_t *handle, int pname, void *param, unsigned int psize);
	/** Module ops: set parameters of a processor, return 0 if succeed */
	int (*set_parameter)(audio_processor_t *handle, int pname, void *param, unsigned int psize);
	/** Module ops: debug a processor, return 0 if succeed */
	int (*dump)(audio_processor_t *handle, int data_tag, struct acts_ringbuf *data_buf);

	/** The following ops are mostly used to send control commands in dsp implemented processors */
#ifdef CONFIG_DSP
	/** Module ops: enable a processor (may cause re-initialization), return 0 if succeed */
	int (*enable)(audio_processor_t *handle);
	/** Module ops: disable a processor, return 0 if succeed */
	int (*disable)(audio_processor_t *handle);
	/** Module ops: pause a processor, return 0 if succeed */
	int (*pause)(audio_processor_t *handle);
	/** Module ops: resum a processor, return 0 if succeed */
	int (*resume)(audio_processor_t *handle);
#endif /* CONFIG_DSP */
} audio_processor_module_t;

/**
 * @brief Open a audio preprocessor.
 *
 * This routine open a audio preprocessor.
 *
 * @param init_param Address of preprocessor init param
 *
 * @return handle of preprocessor
 */
void *audio_preprocessor_open(audio_preprocessor_init_param_t *init_param);

/**
 * @brief Open a audio postprocessor.
 *
 * This routine open a audio postprocessor.
 *
 * @param init_param Address of postprocessor init param
 *
 * @return handle of postprocessor
 */
void *audio_postprocessor_open(audio_postprocessor_init_param_t *init_param);

/**
 * @brief Close a audio pre/postprocessor.
 *
 * This routine close a audio pre/postprocessor.
 *
 * @param handle Handle of pre/postprocessor.
 *
 * @return 0 if succeed; non-zero if fail.
 */
int audio_processor_close(void *handle);

/**
 * @brief Enable a audio pre/postprocessor.
 *
 * This routine enable a audio pre/postprocessor.
 *
 * @param handle Handle of pre/postprocessor.
 *
 * @return 0 if succeed; non-zero if fail.
 */
int audio_processor_enable(void *handle);

/**
 * @brief Disable a audio pre/postprocessor.
 *
 * This routine disable a audio pre/postprocessor.
 *
 * @param handle Handle of pre/postprocessor.
 *
 * @return 0 if succeed; non-zero if fail.
 */
int audio_processor_disable(void *handle);

/**
 * @brief Pause a audio pre/postprocessor.
 *
 * This routine pause a audio pre/postprocessor.
 *
 * @param handle Handle of pre/postprocessor.
 *
 * @return 0 if succeed; non-zero if fail.
 */
int audio_processor_pause(void *handle);

/**
 * @brief Resume a audio pre/postprocessor.
 *
 * This routine resume a audio pre/postprocessor.
 *
 * @param handle Handle of pre/postprocessor.
 *
 * @return 0 if succeed; non-zero if fail.
 */
int audio_processor_resume(void *handle);

/**
 * @brief Run a audio pre/postprocessor to process data.
 *
 * This routine run a audio pre/postprocessor to process data.
 *
 * @param handle Handle of pre/postprocessor.
 *
 * @return number of samples processed.
 */
int audio_processor_process(void *handle);

/**
 * @brief Flush the remain data of a audio pre/postprocessor.
 *
 * This routine flush the remain data of a audio pre/postprocessor.
 *
 * @param handle Handle of pre/postprocessor.
 *
 * @return number of samples processed.
 */
int audio_processor_flush(void *handle);

/**
 * @brief Get parameter of a audio pre/postprocessor.
 *
 * This routine get parameter of a audio pre/postprocessor.
 *
 * @param handle Handle of pre/postprocessor.
 * @param pname  Parameter name.
 * @param param  Address of parameter.
 * @param psize  Size of parameter.
 *
 * @return 0 if succeed; non-zero if fail.
 */
int audio_processor_get_parameter(void *handle, int pname, void *param, unsigned int psize);

/**
 * @brief Set parameter of a audio pre/postprocessor.
 *
 * This routine set parameter of a audio pre/postprocessor.
 *
 * @param handle Handle of pre/postprocessor.
 * @param pname  Parameter name.
 * @param param  Address of parameter.
 * @param psize  Size of parameter.
 *
 * @return 0 if succeed; non-zero if fail.
 */
int audio_processor_set_parameter(void *handle, int pname, void *param, unsigned int psize);

/**
 * @brief Dump a audio pre/postprocessor
 *
 * This routine debug a audio pre/postprocessor.
 *
 * @param handle Handle of pre/postprocessor.
 * @param data_tag tag of dumpable data, see enum media_dump_data_tag_e in media_service.h
 * @param data_buf buffer to store dumpable data
 *
 * @return 0 if succeed; non-zero if fail.
 */
int audio_processor_dump(void *handle, int data_tag, struct acts_ringbuf *data_buf);

/**
 * @brief Register a audio preprocessor.
 *
 * This routine register a audio preprocessor to process data of specific stream_type.
 *
 * @param module Address of preprocessor module
 * @param stream_type type of audio stream, see audio_stream_type_e.
 *
 * @return 0 if succeed; non-zero if fail.
 */
int audio_preprocessor_register(audio_processor_module_t *module, uint8_t stream_type);

/**
 * @brief Unregister a audio preprocessor.
 *
 * This routine unregister a audio preprocessor which process data of specific stream_type.
 *
 * @param module Address of preprocessor module
 * @param stream_type type of audio stream, see audio_stream_type_e.
 *
 * @return 0 if succeed; non-zero if fail.
 */
int audio_preprocessor_unregister(audio_processor_module_t *module, uint8_t stream_type);

/**
 * @brief Register a audio postprocessor.
 *
 * This routine register a audio postprocessor to process data of specific stream_type.
 *
 * @param module Address of postprocessor module
 * @param stream_type type of audio stream, see audio_stream_type_e.
 *
 * @return 0 if succeed; non-zero if fail.
 */
int audio_postprocessor_register(audio_processor_module_t *module, uint8_t stream_type);

/**
 * @brief Unregister a audio postprocessor.
 *
 * This routine unregister a audio postprocessor which process data of specific stream_type.
 *
 * @param module Address of postprocessor module
 * @param stream_type type of audio stream, see audio_stream_type_e.
 *
 * @return 0 if succeed; non-zero if fail.
 */
int audio_postprocessor_unregister(audio_processor_module_t *module, uint8_t stream_type);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __AUDIO_PROCESSOR_H__ */
