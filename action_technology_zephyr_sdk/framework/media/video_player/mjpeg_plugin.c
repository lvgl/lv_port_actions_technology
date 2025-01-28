#include "video_player_defs.h"
#include "vp_engine.h"
#include <soc_dsp.h>
#include "video_mem.h"
#include <jpeg_hal.h>
#include <file_stream.h>

#define IMAGE_BUF_SIZE       	(VIDEO_PIXEL_WIDTH*VIDEO_PIXEL_HEIGHT*2)
#define PADDING_ALIGN_SIZE		1024

#define FRAME_DEC_TIMEOUT		(300*1000)

//static int frame_num = 0;

typedef struct {
	void *dsp_handle;
	ve_video_info_t vinfo;
	struct acts_ringbuf *aud_ringbuf;
} mjpeg_plugin_data_t;

static mjpeg_plugin_data_t *plugin_data = NULL;

static void *open(void *arg)
{
	ve_video_info_t *vinfo = (ve_video_info_t *)arg;
	mjpeg_plugin_data_t *data;

	data = video_mem_malloc(sizeof(mjpeg_plugin_data_t));
	if(data == NULL)
	{
		SYS_LOG_ERR("data malloc error %d\n", __LINE__);
		return NULL;
	}
	memset(data, 0, sizeof(mjpeg_plugin_data_t));
	plugin_data = data;

	memcpy(&plugin_data->vinfo, vinfo, sizeof(ve_video_info_t));

	//frame_num = 0;
	SYS_LOG_INF("ok");

	return data;
}

static int decode(void *dhandle, av_buf_t *av_buf)
{
	mjpeg_plugin_data_t *data = (mjpeg_plugin_data_t *)dhandle;
	ve_video_info_t *vinfo = &data->vinfo;
	int ret;
	uint32_t time_before, time_after;
	time_before = os_uptime_get_32();
	ret = jpg_decode(av_buf->data, av_buf->data_len, av_buf->outbuf, 1, vinfo->width,
		0, 0, vinfo->width, vinfo->height);
	if(ret != vinfo->width * vinfo->height * 2) {
		SYS_LOG_ERR("### _jpg_decode fail %d", ret);
		return EN_DECERR;
	}
	time_after = os_uptime_get_32();
	//SYS_LOG_INF("### frame[%d, %d], time: %d\n", frame_num, ori_len, k_cyc_to_us_floor32(k_cycle_get_32() - start_time));
	//frame_num ++;
	SYS_LOG_DBG("### spend time:%d", time_after - time_before);
	return EN_NORMAL;
}

static int dispose(void *dhandle)
{
	mjpeg_plugin_data_t *data = (mjpeg_plugin_data_t *)dhandle;

	if (data)
		video_mem_free(data);

	plugin_data = NULL;

	return 0;
}

static dec_plugin_t  mjpeg_plugin = {
	"mjpeg",
	open,
	decode,
	dispose
};

dec_plugin_t * mjpeg_api(void)
{
	return &mjpeg_plugin;
}

