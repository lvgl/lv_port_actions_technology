#ifndef H_VP_ENGINE
#define H_VP_ENGINE

#include <stream.h>

#include <jpeg_hal.h>

typedef struct
{
    char video[8];
    unsigned int frame_rate;
    unsigned int width;
    unsigned int height;
    unsigned int video_bitrate;
    unsigned int vtotal_time;
}ve_video_info_t;

typedef struct
{
	char audio[8];
    unsigned int sample_rate;
    unsigned int audio_bitrate;
    unsigned int channels;
    unsigned int atotal_time;
}ve_audio_info_t;

typedef struct
{
	ve_video_info_t video_info;
	ve_audio_info_t audio_info;
	int index_flag;
}ve_media_info_t;


typedef enum{
	GET_CONT_INFO,
	FAST_FORWORD,
	FAST_BACK,
	SEEK_TIME,
}seek_cmd_t;

typedef struct
{
	seek_cmd_t    seek_cmd;
	unsigned int  curpos;
    unsigned int  curframes;
    unsigned int  curtime;
	unsigned char *searchbuf;
}seek_info_t;

typedef struct{
	int w;
	int h;
}image_para_t;

typedef enum{
	VIDEO_ES = 0,
	AUDIO_ES,
	UNKOWN_ES,
}packet_type_t;

typedef struct
{
	unsigned char *data;
	unsigned int data_len;
	io_stream_t file_stream;
	image_para_t src_para;
	image_para_t out_para;
	short *outbuf;
}av_buf_t;

typedef struct
{
	packet_type_t packet_type;
	unsigned int pts;
	unsigned char *data;
	unsigned int data_len;
}demuxer_packet_t;

typedef enum{
	FRAME_NORMAL,
	FRAME_END,
	FRAME_ERROR,
}frame_status_t;

typedef struct
{
	av_buf_t av_dec_buf;
	frame_status_t status;
	packet_type_t packet_type;
	unsigned int pts;
}frame_info_t;

typedef struct
{
	char file_extension[8];
	void *(*open)(io_stream_t stream, ve_media_info_t *file_info);
	int (*seek)(void *vp_handle, seek_info_t *seek_info);
	int (*getframe)(void *vp_handle, demuxer_packet_t *packet_buf);
	int (*dispose)(void *vp_handle);
}demuxer_plugin_t;

typedef struct
{
	char file_extension[8];
	void *(*open)(void *arg);
	int (*decode)(void *vp_handle, av_buf_t *av_buf);
	int (*dispose)(void *vp_handle);
}dec_plugin_t;


demuxer_plugin_t * avi_api(void);
dec_plugin_t * mjpeg_api(void);
dec_plugin_t *adpcm_api(void);

#endif
