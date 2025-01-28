/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <stdint.h>
#include "dsp_inner.h"

static struct dsp_ringbuf *buf_dsp2cpu(uint32_t dsp_buf)
{
	return (struct dsp_ringbuf *)dsp_data_to_mcu_address(dsp_buf);
}

static void print_ringbuf_info(const char *str, uint32_t buf, int type)
{
	struct dsp_ringbuf *dsp_buf = buf_dsp2cpu(buf);

	if (type == 0) {
		printk("\t\t%s=%p (length=%u/%u ptr=%u)\n",
			str, dsp_buf,
			dsp_ringbuf_length(dsp_buf),
			dsp_ringbuf_size(dsp_buf),
			(u32_t)acts_ringbuf_base(&dsp_buf->buf));
	} else {
		printk("\t\t%s=%p (space=%u/%u ptr=%u)\n",
			str, dsp_buf,
			dsp_ringbuf_space(dsp_buf),
			dsp_ringbuf_size(dsp_buf),
			(u32_t)acts_ringbuf_base(&dsp_buf->buf));
	}
}

static void dump_media_session(void *info)
{
#if 0
	struct media_dspssn_info *media_info = info;
	struct media_dspssn_params *params = &media_info->params;
	printk("\nsession media (id=%u):\n", DSP_SESSION_MEDIA);
	printk("\taec_en=%d\n", params->aec_en);
	printk("\taec_delay=%d samples\n", params->aec_delay);
	if (media_info->aec_refbuf)
		printk("\taec_refbuf=%p (length=%u/%u)\n",
		       buf_dsp2cpu(media_info->aec_refbuf),
		       dsp_ringbuf_length(buf_dsp2cpu(media_info->aec_refbuf)),
		       dsp_ringbuf_size(buf_dsp2cpu(media_info->aec_refbuf)));
#endif
}

void dsp_session_dump_info(struct dsp_session *session, void *info)
{
	switch (session->id) {
	case DSP_SESSION_MEDIA:
		dump_media_session(info);
		break;
	default:
		break;
	}
}

#if 0
static void dump_test_function(void *info, bool dump_debug)
{
	struct test_dspfunc_info *test_info = info;
	struct test_dspfunc_params *params = &test_info->params;
	struct test_dspfunc_runtime *runtime = &test_info->runtime;

	printk("\nfunction (id=%u):\n", DSP_FUNCTION_TEST);
	printk("\tparameters:\n");
	printk("\t\t(%p)inbuf.length=%u/%u\n", buf_dsp2cpu(params->inbuf),
	       dsp_ringbuf_length(buf_dsp2cpu(params->inbuf)),
	       dsp_ringbuf_size(buf_dsp2cpu(params->inbuf)));
	printk("\t\t(%p)outbuf.space=%u/%u\n", buf_dsp2cpu(params->outbuf),
	       dsp_ringbuf_space(buf_dsp2cpu(params->outbuf)),
	       dsp_ringbuf_size(buf_dsp2cpu(params->outbuf)));

	printk("\truntime:\n");
	printk("\t\tsample_count=%u\n", runtime->sample_count);
}
#endif

static void dump_decoder_function(void *info, bool dump_debug)
{
	struct decoder_dspfunc_info *decoder_info = info;
	struct decoder_dspfunc_params *params = &decoder_info->params;
	struct decoder_dspfunc_runtime *runtime = &decoder_info->runtime;
	struct decoder_dspfunc_debug *debug = &decoder_info->debug;

	printk("\nfunction decoder (id=%u):\n", DSP_FUNCTION_DECODER);
	printk("\tparameters:\n");
	printk("\t\tformat=%u\n", params->format);
	printk("\t\tsample_rate(in)=%u\n", params->resample_param.input_sr);
	printk("\t\tsample_rate(out)=%u\n", params->resample_param.output_sr);
	print_ringbuf_info("inbuf", params->inbuf, 0);
	print_ringbuf_info("outbuf", params->outbuf, 1);

	printk("\truntime:\n");
	printk("\t\tframe_size=%u\n", runtime->frame_size);
	printk("\t\tchannels=%u\n", runtime->channels);
	printk("\t\tsample_count=%u\n", runtime->sample_count);
	printk("\t\tdatalost_count=%u\n", runtime->datalost_count);
	printk("\t\traw_count=%u\n", runtime->raw_count);

	if (!dump_debug)
		return;

	printk("\tdebug:\n");
	if (debug->stream) {
		print_ringbuf_info("stream", debug->stream, 1);
	}
	if (debug->pcmbuf) {
		print_ringbuf_info("pcmbuf", debug->stream, 1);
	}
}

static void dump_encoder_function(void *info, bool dump_debug)
{
	struct encoder_dspfunc_info *encoder_info = info;
	struct encoder_dspfunc_params *params = &encoder_info->params;
	struct encoder_dspfunc_runtime *runtime = &encoder_info->runtime;
	struct encoder_dspfunc_debug *debug = &encoder_info->debug;

	printk("\nfunction encoder (id=%u):\n", DSP_FUNCTION_ENCODER);
	printk("\tparameters:\n");
	printk("\t\tformat=%u\n", params->format);
	printk("\t\tsample_rate=%u\n", params->sample_rate);
	printk("\t\tbit_rate=%u\n", params->bit_rate);
	printk("\t\tcomplexityt=%u\n", params->complexity);
	printk("\t\tchannels=%u\n", params->channels);
	printk("\t\tframe_size=%u\n", params->frame_size);
	print_ringbuf_info("inbuf", params->inbuf, 0);
	print_ringbuf_info("outbuf", params->outbuf, 1);

	printk("\truntime:\n");
	printk("\t\tframe_size=%u\n", runtime->frame_size);
	printk("\t\tchannels=%u\n", runtime->channels);
	printk("\t\tcompression_ratio=%u\n", runtime->compression_ratio);
	printk("\t\tsample_count=%u\n", runtime->sample_count);

	if (!dump_debug)
		return;

	printk("\tdebug:\n");
	if (debug->pcmbuf) {
		print_ringbuf_info("pcmbuf", debug->pcmbuf, 1);
	}
	if (debug->stream) {
		print_ringbuf_info("stream", debug->stream, 1);
	}
}

static void dump_player_function(void *info, bool dump_debug)
{
	struct player_dspfunc_info *player_info = info;
	struct decoder_dspfunc_info *decoder_info =
			(void *)dsp_data_to_mcu_address(POINTER_TO_UINT(player_info->decoder_info));
	struct player_dspfunc_debug *debug = &player_info->debug;

	printk("\nfunction player (id=%u):\n", DSP_FUNCTION_PLAYER);
	printk("\tparameters:\n");
	printk("\t\tdae.pbuf=%p\n", (void *)dsp_data_to_mcu_address(player_info->dae.pbuf));
	printk("\t\tdae.pbufsz=%u\n", player_info->dae.pbufsz);

#if 0
	if (player_info->aec.enable) {
		printk("\t\taec.delay=%d\n", player_info->aec.delay);
		printk("\t\taec.refbuf=%p (space=%u/%u)\n",
			buf_dsp2cpu(player_info->aec.channel[0].refbuf),
			dsp_ringbuf_space(buf_dsp2cpu(player_info->aec.channel[0].refbuf)),
			dsp_ringbuf_size(buf_dsp2cpu(player_info->aec.channel[0].refbuf)));
	}
#endif

	print_ringbuf_info("dacbuf", player_info->output.outbuf, 1);

	if (!dump_debug) {
		dump_decoder_function(decoder_info, false);
		return;
	}

	printk("\tdebug:\n");
	if (debug->decode_stream) {
		print_ringbuf_info("decode_stream", debug->decode_stream, 1);
	}
	if (debug->decode_data) {
		print_ringbuf_info("decode_data", debug->decode_data, 1);
	}
	if (debug->plc_data) {
		print_ringbuf_info("plc_data", debug->plc_data, 1);
	}

	dump_decoder_function(decoder_info, false);
}

static void dump_asr_function(void *info, bool dump_debug)
{
	struct asr_dspfunc_info *asr_info = info;
	struct asr_dspfunc_params *params = &asr_info->params;
	struct asr_dspfunc_runtime *runtime = &asr_info->runtime;
	struct asr_dspfunc_debug *debug = &asr_info->debug;

	printk("\nfunction asr (id=%u):\n", DSP_FUNCTION_ASR);
	printk("\tparameters:\n");
	printk("\t\tsample_rate=%u\n", params->sample_rate);
	printk("\t\tinbuf=%p (length=%u/%u)\n",
			buf_dsp2cpu(params->inbuf),
			dsp_ringbuf_length(buf_dsp2cpu(params->inbuf)),
			dsp_ringbuf_size(buf_dsp2cpu(params->inbuf)));
	if (params->outbuf) {
		printk("\t\toutbuf=%p (space=%u/%u)\n",
				buf_dsp2cpu(params->outbuf),
				dsp_ringbuf_space(buf_dsp2cpu(params->outbuf)),
				dsp_ringbuf_size(buf_dsp2cpu(params->outbuf)));
	} else {
		printk("\t\toutbuf=null\n");
	}

	printk("\truntime:\n");
	printk("\t\tframe_size=%u\n", runtime->frame_size);
	printk("\t\tsample_count=%u\n", runtime->sample_count);
	printk("\t\tasr_id=%d\n", runtime->asr_id);
	printk("\t\tasr_count=%u\n", runtime->asr_count);

	if (!dump_debug)
		return;

	printk("\tdebug:\n");
	if (debug->pcmbuf) {
		printk("\t\tpcmbuf=%p (space=%u/%u)\n",
		       buf_dsp2cpu(debug->pcmbuf),
		       dsp_ringbuf_space(buf_dsp2cpu(debug->pcmbuf)),
		       dsp_ringbuf_size(buf_dsp2cpu(debug->pcmbuf)));
	}
	if (debug->resbuf) {
		printk("\t\tstream=%p (space=%u/%u)\n",
		       buf_dsp2cpu(debug->resbuf),
		       dsp_ringbuf_space(buf_dsp2cpu(debug->resbuf)),
		       dsp_ringbuf_size(buf_dsp2cpu(debug->resbuf)));
	}
}

static void dump_recorder_function(void *info, bool dump_debug)
{
	struct recorder_dspfunc_info *recorder_info = info;
	struct recorder_dspfunc_params *params = &recorder_info->param;
	struct recorder_dspfunc_debug *debug = &recorder_info->debug;
	struct asr_dspfunc_info *asr_info =
		(void *)dsp_data_to_mcu_address(recorder_info->asr_info);


	struct encoder_dspfunc_info *encoder_info =
		(void *)dsp_data_to_mcu_address((u32_t)recorder_info->encoder_info);

	printk("\nfunction recorder (id=%u):\n", DSP_FUNCTION_RECORDER);
	printk("\tparameters: %p params %p\n",recorder_info,params);


	printk("\t\tmode=%d\n", params->mode);
	if (params->evtbuf) {
		printk("\t\tevtbuf=%p (length=%u/%u)\n",
			   buf_dsp2cpu(params->evtbuf),
			   dsp_ringbuf_length(buf_dsp2cpu(params->evtbuf)),
			   dsp_ringbuf_size(buf_dsp2cpu(params->evtbuf)));
	}

	printk("\t\tdae\n");
	printk("\t\t - pbuf=%p\n", (void *)buf_dsp2cpu(recorder_info->dae.pbuf));
	printk("\t\t - pbufsz=%u\n", recorder_info->dae.pbufsz);


	if (recorder_info->aec_ref.channel[0].refbuf) {
		printk("\t\taec.delay=%d\n", recorder_info->aec_ref.delay);
		printk("\t\taec.refbuf=%p (space=%u/%u)\n",
			buf_dsp2cpu(recorder_info->aec_ref.channel[0].refbuf),
			dsp_ringbuf_space(buf_dsp2cpu(recorder_info->aec_ref.channel[0].refbuf)),
			dsp_ringbuf_size(buf_dsp2cpu(recorder_info->aec_ref.channel[0].refbuf)));
	}

	printk("\t\tvad\n");
	printk("\t\t - enabled=%u\n", recorder_info->vad.enabled);
	if (recorder_info->vad.enabled) {
		printk("\t\t - sample_rate=%u\n", recorder_info->vad.sample_rate);
		printk("\t\t - channels=%u\n", recorder_info->vad.channels);
		printk("\t\t - amplitude=%u\n", recorder_info->vad.amplitude);
		printk("\t\t - energy=%u\n", recorder_info->vad.energy);
		printk("\t\t - decibel=%u\n", recorder_info->vad.decibel);
		printk("\t\t - period=%u\n", recorder_info->vad.period);
		printk("\t\t - uptime=%u\n", recorder_info->vad.uptime);
		printk("\t\t - downtime=%u\n", recorder_info->vad.downtime);
		printk("\t\t - slowadapt=%u\n", recorder_info->vad.slowadapt);
		printk("\t\t - fastadapt=%u\n", recorder_info->vad.fastadapt);
		printk("\t\t - agc=%u\n", recorder_info->vad.agc);
		printk("\t\t - ns=%u\n", recorder_info->vad.ns);
		printk("\t\t - outputzero=%u\n", recorder_info->vad.outputzero);
	}

	printk("\t\tadcbuf=%p (length=%u/%u)\n",
	       buf_dsp2cpu(recorder_info->adcbuf),
	       dsp_ringbuf_space(buf_dsp2cpu(recorder_info->adcbuf)),
	       dsp_ringbuf_size(buf_dsp2cpu(recorder_info->adcbuf)));

	if (!dump_debug) {
		dump_encoder_function(encoder_info, false);
		return;
	}

	printk("\tdebug:\n");
	if (debug->mic1_data) {
		printk("\t\tmic1_data=%p (space=%u/%u)\n",
		       buf_dsp2cpu(debug->mic1_data),
		       dsp_ringbuf_space(buf_dsp2cpu(debug->mic1_data)),
		       dsp_ringbuf_size(buf_dsp2cpu(debug->mic1_data)));
	}
	if (debug->mic2_data) {
		printk("\t\tmic2_data=%p (space=%u/%u)\n",
		       buf_dsp2cpu(debug->mic2_data),
		       dsp_ringbuf_space(buf_dsp2cpu(debug->mic2_data)),
		       dsp_ringbuf_size(buf_dsp2cpu(debug->mic2_data)));
	}
	if (debug->ref_data) {
		printk("\t\tref_data=%p (space=%u/%u)\n",
		       buf_dsp2cpu(debug->ref_data),
		       dsp_ringbuf_space(buf_dsp2cpu(debug->ref_data)),
		       dsp_ringbuf_size(buf_dsp2cpu(debug->ref_data)));
	}
	if (debug->aec1_data) {
		printk("\t\taec1_data=%p (space=%u/%u)\n",
		       buf_dsp2cpu(debug->aec1_data),
		       dsp_ringbuf_space(buf_dsp2cpu(debug->aec1_data)),
		       dsp_ringbuf_size(buf_dsp2cpu(debug->aec1_data)));
	}
	if (debug->aec2_data) {
		printk("\t\taec2_data=%p (space=%u/%u)\n",
		       buf_dsp2cpu(debug->aec2_data),
		       dsp_ringbuf_space(buf_dsp2cpu(debug->aec2_data)),
		       dsp_ringbuf_size(buf_dsp2cpu(debug->aec2_data)));
	}
	if (debug->encode_data) {
		printk("\t\tencode_data=%p (space=%u/%u)\n",
		       buf_dsp2cpu(debug->encode_data),
		       dsp_ringbuf_space(buf_dsp2cpu(debug->encode_data)),
		       dsp_ringbuf_size(buf_dsp2cpu(debug->encode_data)));
	}
	if (debug->encode_stream) {
		printk("\t\tencode_stream=%p (space=%u/%u)\n",
		       buf_dsp2cpu(debug->encode_stream),
		       dsp_ringbuf_space(buf_dsp2cpu(debug->encode_stream)),
		       dsp_ringbuf_size(buf_dsp2cpu(debug->encode_stream)));
	}

	if (params->mode == RECORDER_DSPMODE_ENCODE)
		dump_encoder_function(encoder_info, false);
	else
		dump_asr_function(asr_info, false);

}

void dsp_session_dump_function(struct dsp_session *session, unsigned int func)
{
	struct dsp_request_function request = { .id = func, };
	dsp_request_userinfo(session->dev, DSP_REQUEST_FUNCTION_INFO, &request);

	if (request.info == NULL)
		return;

	switch (func) {
#if 0
	case DSP_FUNCTION_TEST:
		dump_test_function(request.info, true);
		break;
#endif
	case DSP_FUNCTION_ENCODER:
		dump_encoder_function(request.info, true);
		break;
	case DSP_FUNCTION_DECODER:
		dump_decoder_function(request.info, true);
		break;
	case DSP_FUNCTION_PLAYER:
		dump_player_function(request.info, true);
		break;
	case DSP_FUNCTION_RECORDER:
		dump_recorder_function(request.info, true);
		break;
	default:
		break;
	}
}

unsigned int dsp_session_get_samples_count(struct dsp_session *session, unsigned int func)
{
	union {
		struct test_dspfunc_info *test;
		struct encoder_dspfunc_info *encoder;
		struct decoder_dspfunc_info *decoder;
	} func_info;

	if (func == DSP_FUNCTION_PLAYER)
		func = DSP_FUNCTION_DECODER;
	else if (func == DSP_FUNCTION_RECORDER)
		func = DSP_FUNCTION_ENCODER;

	struct dsp_request_function request = { .id = func, };
	dsp_request_userinfo(session->dev, DSP_REQUEST_FUNCTION_INFO, &request);
	if (request.info == NULL)
		return 0;

	switch (func) {
#if 0
	case DSP_FUNCTION_TEST:
		func_info.test = request.info;
		return func_info.test->runtime.sample_count;
#endif
	case DSP_FUNCTION_ENCODER:
		func_info.encoder = request.info;
		return func_info.encoder->runtime.sample_count;
	case DSP_FUNCTION_DECODER:
		func_info.decoder = request.info;
		return func_info.decoder->runtime.sample_count;
	default:
		return 0;
	}
}

unsigned int dsp_session_get_datalost_count(struct dsp_session *session, unsigned int func)
{
	struct dsp_request_function request = { .id = DSP_FUNCTION_DECODER, };

	if (func != DSP_FUNCTION_PLAYER && func != DSP_FUNCTION_DECODER)
		return 0;

	dsp_request_userinfo(session->dev, DSP_REQUEST_FUNCTION_INFO, &request);
	if (request.info) {
		struct decoder_dspfunc_info *decoder = request.info;
		return decoder->runtime.datalost_count;
	}

	return 0;
}

unsigned int dsp_session_get_raw_count(struct dsp_session *session, unsigned int func)
{
	struct dsp_request_function request = { .id = DSP_FUNCTION_DECODER, };

	if (func != DSP_FUNCTION_PLAYER && func != DSP_FUNCTION_DECODER)
		return 0;

	dsp_request_userinfo(session->dev, DSP_REQUEST_FUNCTION_INFO, &request);
	if (request.info) {
		struct decoder_dspfunc_info *decoder = request.info;
		return decoder->runtime.raw_count;
	}

	return 0;
}

int dsp_session_get_recoder_param(struct dsp_session *session, int param_type, void *param)
{
	struct dsp_request_session session_request;

	struct dsp_request_function request = { .id = DSP_FUNCTION_RECORDER, };

	dsp_request_userinfo(session->dev, DSP_REQUEST_SESSION_INFO, &session_request);

	if (!(session_request.func_enabled & DSP_FUNC_BIT(DSP_FUNCTION_RECORDER))) {
		return -EINVAL;
	}

	dsp_request_userinfo(session->dev, DSP_REQUEST_FUNCTION_INFO, &request);
	if (request.info) {
		struct recorder_dspfunc_info *encoder = request.info;
		if (param_type == DSP_CONFIG_AEC) {
			memcpy(param, &encoder->aec, sizeof(struct aec_dspfunc_params));
			return 0;
		}
	}

	return -EINVAL;
}
