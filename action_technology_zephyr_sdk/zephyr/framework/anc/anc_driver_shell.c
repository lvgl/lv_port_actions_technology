

#include "shell/shell.h"
#include "drivers/anc.h"
#include "anc_hal.h"
#include <media_mem.h>
#include "stream.h"
#include "ringbuff_stream.h"
#include "utils/acts_ringbuf.h"
#include "audio_system.h"
#include "soc_anc.h"
#include "drivers/audio/audio_out.h"

#define DATA_FRAME_SIZE 64

static int dump_len = 0;
static os_timer dump_timer;
static void *dump_ringbuf = NULL;
static io_stream_t dump_stream;

static int cmd_open_anc(const struct shell *shell,
			      size_t argc, char **argv)
{
	if(argc != 2){
		SYS_LOG_ERR("usage: anc open <mode>");
		return 0;
	}

	return anc_dsp_open(atoi(argv[1]));
}


static int cmd_close_anc(const struct shell *shell,
			      size_t argc, char **argv)

{
	anc_dsp_close();
	return 0;
}

static void _anc_dump_data_timer_handler(os_timer *ttimer)
{
	static int cnt = 0;
	int ret = 0, i = 0;
	char data[DATA_FRAME_SIZE];

	if(dump_ringbuf ==NULL)
		return;

	for(i=0; i<5; i++){
		ret = acts_ringbuf_length(dump_ringbuf);
		if(ret < DATA_FRAME_SIZE)
			break;

		dump_len += acts_ringbuf_get(dump_ringbuf, data, DATA_FRAME_SIZE);

	}

	cnt++;
	if(cnt == 1000){
		cnt = 0;
		SYS_LOG_INF("get %d bytes data total", dump_len);
	}

	return;
}

static int cmd_dump_data_start(const struct shell *shell,
			      size_t argc, char **argv)

{	char *dumpbuf;
	void *ringbuf;
	int bufsize;

	dumpbuf = media_mem_get_cache_pool(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_MUSIC);
	bufsize = media_mem_get_cache_pool_size(TOOL_ASQT_DUMP_BUF, AUDIO_STREAM_MUSIC);

	SYS_LOG_INF("buf 0x%x, size 0x%x", dumpbuf, bufsize);

	dump_stream = ringbuff_stream_create_ext(dumpbuf, bufsize);

	dump_ringbuf = stream_get_ringbuffer(dump_stream);
	SYS_LOG_INF("ringbuf mcu addr 0x%x", dump_ringbuf);

	ringbuf = mcu_to_anc_address(dump_ringbuf, 0);
	SYS_LOG_INF("ringbuf anc addr 0x%x", ringbuf);

	dump_len = 0;
	os_timer_init(&dump_timer, _anc_dump_data_timer_handler, NULL);
	os_timer_start(&dump_timer, K_MSEC(2), K_MSEC(2));
	anc_dsp_dump_data(1, ringbuf);
	return 0;
}

static int cmd_dump_data_stop(const struct shell *shell,
			      size_t argc, char **argv)

{
	os_timer_stop(&dump_timer);
	anc_dsp_dump_data(0, NULL);
	stream_close(dump_stream);
	stream_destroy(dump_stream);
	dump_ringbuf = NULL;
	return 0;
}

static int cmd_sample_rate_change(const struct shell *shell,
			      size_t argc, char **argv)

{
	struct device *anc_dev = NULL;
	struct device *aout_dev = NULL;
	uint32_t samplerete = 48000, dac_digctl = 0;

	anc_dev = device_get_binding(CONFIG_ANC_NAME);
	aout_dev = device_get_binding(CONFIG_AUDIO_OUT_ACTS_DEV_NAME);
	if(!aout_dev || !aout_dev)
	{
		SYS_LOG_ERR("get device failed\n");
		return -1;
	}

	if(anc_get_status(anc_dev) != ANC_STATUS_POWERON)
	{
		SYS_LOG_ERR("anc not open");
		return 0;
	}

	/*get dac samplerate*/
	audio_out_control(aout_dev, NULL, AOUT_CMD_GET_SAMPLERATE, &samplerete);
	if(samplerete == 0){
		SYS_LOG_ERR("get sample rate err");
	}

	/*get DAC_DIGCTL register value*/
	dac_digctl = sys_read32(AUDIO_DAC_REG_BASE + 0x0);

	SYS_LOG_INF("samplerate %d, dac_digctl 0x%x",samplerete, dac_digctl);

	return anc_fs_change(anc_dev, samplerete, dac_digctl);
}


SHELL_STATIC_SUBCMD_SET_CREATE(sub_acts_anc,
	SHELL_CMD(open, NULL, "init and open anc", cmd_open_anc),
	SHELL_CMD(close, NULL, "deinit and close anc", cmd_close_anc),
	SHELL_CMD(dump_start, NULL, "anc dsp start dump data", cmd_dump_data_start),
	SHELL_CMD(dump_stop, NULL, "anc dsp stop dump data", cmd_dump_data_stop),
	SHELL_CMD(sr_change, NULL, "set new sample rate", cmd_sample_rate_change),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(anc, &sub_acts_anc, "Actions anc commands", NULL);
