#include <os_common_api.h>
#include <audio_system.h>
#include <audio_record.h>
#include <audio_track.h>
#include <ringbuff_stream.h>
#include <stream.h>
#include <soc_dsp.h>
#include "aic_srv_voice.h"
#include <media_type.h>
#include <drivers/hrtimer.h>
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define CONFIG_VOICE_WORK_Q_PRIORITY -2

/****************************************************************************
 * Private Types
 ****************************************************************************/
struct aic_call_voice_player {
	struct audio_record_t *mic;
	struct acts_ringbuf *input_ringbuf;
	io_stream_t audio_stream_in;

	struct audio_track_t *speaker;
	struct acts_ringbuf *output_ringbuf;
	io_stream_t audio_stream_out;

	struct acts_ringbuf *cache_ringbuf;

	struct k_work voice_work;
	struct hrtimer voice_timer;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/


/****************************************************************************
 * Private Data
 ****************************************************************************/

static char record_pcm_buffer[640 * 2];
static char play_pcm_buffer[640 * 4];
static struct k_work_q voice_workq;
static char intput_cache_buffer[640];
static char output_cache_buffer[640];
static bool g_voice_start_flag = false;

static K_THREAD_STACK_DEFINE(voice_workq_stack, 2048);

static struct aic_call_voice_player voice_player;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void voice_work_handle(os_work *work);
static void voice_timer_handle(struct hrtimer *timer, void *expiry_fn_arg);
static int voice_workq_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	k_work_queue_start(&voice_workq, voice_workq_stack,
		       K_THREAD_STACK_SIZEOF(voice_workq_stack),
		       CONFIG_VOICE_WORK_Q_PRIORITY, NULL);

	k_thread_name_set(&voice_workq.thread, "voice_workq");

	os_work_init(&voice_player.voice_work, voice_work_handle);
	hrtimer_init(&voice_player.voice_timer, voice_timer_handle, NULL);
	return 0;
}

SYS_INIT(voice_workq_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/****************************************************************************
 * Public Functions
 ****************************************************************************/
static void voice_work_handle(struct k_work *work)
{
	int try_cnt = 0;
    int input_size = 640;
    int output_size = 640;

	if (input_size > sizeof(intput_cache_buffer)) {
		input_size = sizeof(intput_cache_buffer);
	}

	if (!voice_player.audio_stream_in || !voice_player.audio_stream_out)  {
		return;
	}

retry:
	input_size = stream_read(voice_player.audio_stream_in, intput_cache_buffer, input_size);
	if (input_size <= 0) {
		if(try_cnt ++ > 2) {
			SYS_LOG_INF("input_size %d wrong", input_size);
			return;
		} else{
			os_sleep(2);
			goto retry;
		}
	}

	aic_srv_voice_exchange_data(intput_cache_buffer, &input_size, output_cache_buffer, &output_size, 0);
	audio_aps_monitor(stream_get_length(voice_player.audio_stream_out) / 2 / 16);


	if (output_size > sizeof(output_cache_buffer)) {
		output_size = sizeof(output_cache_buffer);
	}

	stream_write(voice_player.audio_stream_out, output_cache_buffer, output_size);

}

static void voice_timer_handle(struct hrtimer *timer, void *expiry_fn_arg)
{
    os_work_submit_to_queue(&voice_workq, &voice_player.voice_work);
	return;
}

int aic_call_voice_start(void)
{
    if(g_voice_start_flag == true) {
        alog_info("[%s] voice start already, return", __func__);
        return 0;
    }

    g_voice_start_flag = true;

	voice_player.input_ringbuf = acts_ringbuf_init_ext(record_pcm_buffer, sizeof(record_pcm_buffer));
	if (voice_player.input_ringbuf == NULL) {
		goto fail;
	}

	voice_player.mic = audio_record_create(AUDIO_STREAM_MIC_IN, 16, 16,
	AUDIO_FORMAT_PCM_16_BIT, AUDIO_MODE_MONO, (void *)voice_player.input_ringbuf);
	if (voice_player.mic == NULL) {
		goto fail;
	}

	voice_player.audio_stream_in = audio_record_get_stream(voice_player.mic);
	if (voice_player.audio_stream_in == NULL) {
		goto fail;
	}

	voice_player.output_ringbuf = acts_ringbuf_init_ext(play_pcm_buffer, sizeof(play_pcm_buffer));
	if (voice_player.output_ringbuf == NULL) {
		goto fail;
	}

	voice_player.speaker = audio_track_create(AUDIO_STREAM_TTS, 16, AUDIO_FORMAT_PCM_16_BIT,
	AUDIO_MODE_MONO, voice_player.output_ringbuf, NULL, NULL);
	if (voice_player.speaker == NULL) {
		goto fail;
	}

	voice_player.audio_stream_out = audio_track_get_stream(voice_player.speaker);
	if (voice_player.audio_stream_out == NULL) {
		goto fail;
	}

	memset(output_cache_buffer, 0, sizeof(output_cache_buffer));
	stream_write(voice_player.audio_stream_out, output_cache_buffer, sizeof(output_cache_buffer));
	stream_write(voice_player.audio_stream_out, output_cache_buffer, sizeof(output_cache_buffer));
	stream_write(voice_player.audio_stream_out, output_cache_buffer, sizeof(output_cache_buffer));
	stream_write(voice_player.audio_stream_out, output_cache_buffer, sizeof(output_cache_buffer));
	audio_record_start(voice_player.mic);
	audio_track_start(voice_player.speaker);

	hrtimer_start(&voice_player.voice_timer, 20000, 20000);

	audio_aps_monitor_init(PCM_TYPE, NULL, voice_player.speaker);

	return 0;
fail:
	if (voice_player.mic) {
		audio_record_destory(voice_player.mic);
	}

	if (voice_player.speaker) {
		audio_track_destory(voice_player.speaker);
	}

	return -1;
}

int aic_call_voice_stop(void)
{
    if(g_voice_start_flag == false) {
        alog_info("[%s] voice stop already, return", __func__);
        return 0;
    }

    g_voice_start_flag = false;

	hrtimer_stop(&voice_player.voice_timer);

	if (voice_player.mic) {
		audio_record_stop(voice_player.mic);
		audio_record_destory(voice_player.mic);
		voice_player.audio_stream_in = NULL;
		voice_player.mic = NULL;
	}

	if (voice_player.speaker) {
		audio_track_stop(voice_player.speaker);
		audio_track_destory(voice_player.speaker);
		voice_player.audio_stream_out = NULL;
		voice_player.speaker = NULL;
	}

	return 0;
}

