#ifndef AUDIO_TEST_H
#define AUDIO_TEST_H
#include <msg_manager.h>

enum USERAPP_EVENT_TYPE {
    /* test player message */
	MSG_USERAPP_EVENT = MSG_APP_MESSAGE_START,
};

enum USERAPP_CMD {
	NC_START,
	NC_MP3_START,
	NC_STOP,
	NC_GET_VAD,
};

void nc_start(char *save_url);
void nc_mp3_start(char *save_url);
void nc_stop(void);
void nc_get_vad(void);

#endif

