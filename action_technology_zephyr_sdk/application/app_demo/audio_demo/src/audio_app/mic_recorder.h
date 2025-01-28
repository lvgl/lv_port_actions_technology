#ifndef MIC_RECORDER_H
#define MIC_RECORDER_H

typedef struct{
	struct audio_record_t *audio_record;
	struct acts_ringbuf *ringbuf_audio_input;
	io_stream_t audio_stream;
} t_mic_recorder;

t_mic_recorder *mic_recorder_open(char *record_buffer, int record_buffer_size);
int mic_recorder_close(t_mic_recorder *handle);
int mic_recorder_start(t_mic_recorder *handle);
int mic_recorder_stop(t_mic_recorder *handle);

#endif

