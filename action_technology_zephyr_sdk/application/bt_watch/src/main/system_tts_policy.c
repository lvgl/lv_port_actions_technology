/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system tts policy
 */

#include <os_common_api.h>
#include <mem_manager.h>
#include <msg_manager.h>
#include <system_app.h>
#include <string.h>
#include <tts_manager.h>
#ifdef CONFIG_PLAYTTS
#include <audio_system.h>
#endif

const tts_ui_event_map_t  tts_ui_event_map[] = {

	{ UI_EVENT_POWER_ON,			4,  "welcome.mp3",      },
	{ UI_EVENT_POWER_OFF,			4,  "poweroff.mp3",     },
	{ UI_EVENT_STANDBY,				0,  "standby.mp3",      },
	{ UI_EVENT_WAKE_UP,				0,  "wakeup.mp3",       },
	{ UI_EVENT_LOW_POWER,			0,  "batlow.mp3",       },
	{ UI_EVENT_NO_POWER,			0,  "battlo.mp3",       },

	{ UI_EVENT_ENTER_PAIRING,			0,  "entpr.mp3",	},
	{ UI_EVENT_CLEAR_PAIRED_LIST,		0,  "clrpl.mp3",	},
	{ UI_EVENT_WAIT_CONNECTION,			0,  "btwpr.mp3",	},
	{ UI_EVENT_CONNECT_SUCCESS,			0,  "btcntd.mp3",	},
	{ UI_EVENT_SECOND_DEVICE_CONNECT_SUCCESS,	0,  "btcntd2.mp3",},
	{ UI_EVENT_BT_DISCONNECT,			0,  "btdisc.mp3",	},
	{ UI_EVENT_TWS_START_TEAM,			0,  "twswpr.mp3",	},
	{ UI_EVENT_TWS_TEAM_SUCCESS,		0,  "twscntd.mp3",	},
	{ UI_EVENT_TWS_DISCONNECTED,		0,  "twsdisc.mp3",	},

	{ UI_EVENT_BT_INCOMING,				0,  "incoming.mp3",	},
	{ UI_EVENT_START_SIRI,				0,  "tone.mp3",	},
	{ UI_EVENT_MIC_MUTE,				0,  "mute.mp3",		},
	{ UI_EVENT_MIC_MUTE_OFF,			0,  "unmute.mp3",	},
	{ UI_EVENT_BT_CALLRING,				0,  "callring.mp3"	},

	{ UI_EVENT_MIN_VOLUME,				4,  "keytone.pcm",	},
	{ UI_EVENT_MAX_VOLUME,				4,  "keytone.pcm",	},

#ifdef CONFIG_CONTROL_TTS_ENABLE
	{ UI_EVENT_PLAY_START,				0,  "play.mp3",	},
	{ UI_EVENT_PLAY_PAUSE,				0,  "pause.mp3",	},
	{ UI_EVENT_PLAY_PREVIOUS,			0,  "prev.mp3",	},
	{ UI_EVENT_PLAY_NEXT,				0,  "next.mp3",	},
#endif

	{ UI_EVENT_ENTER_LINEIN,			0,  "linein.mp3",	},
	{ UI_EVENT_ENTER_BTMUSIC,			0,  "btmuspl.mp3",	},
	{ UI_EVENT_ENTER_USOUND,			0,  "usound.mp3",	},
	{ UI_EVENT_ENTER_SPDIF_IN,			0,  "spdif.mp3"	},
	{ UI_EVENT_ENTER_I2SRX_IN,			0,  "i2srx.mp3"	},
	{ UI_EVENT_ENTER_SDMPLAYER,			0,  "sdplay.mp3",	},
	{ UI_EVENT_ENTER_UMPLAYER,			0,  "uplay.mp3",	},
	{ UI_EVENT_ENTER_NORMPLAYER,		0,  "norplay.mp3",	},
	{ UI_EVENT_ENTER_SDPLAYBACK,		0,  "sdplaybk.mp3",	},
	{ UI_EVENT_ENTER_UPLAYBACK,			0,  "uplaybk.mp3",	},
	{ UI_EVENT_ENTER_RECORD,			0,  "record.mp3",	},
	{ UI_EVENT_ENTER_MIC_IN,			0,  "mic_in.mp3",	},
	{ UI_EVENT_ENTER_FM,				0,  "fm.mp3",		},

};
#ifdef CONFIG_PLAYTTS
const struct tts_config_t user_tts_config = {
	.tts_storage_media = TTS_SOTRAGE_SDCARD,
	.tts_map = tts_ui_event_map,
	.tts_map_size = sizeof(tts_ui_event_map) / sizeof(tts_ui_event_map_t),
	.tts_def_path = CONFIG_DSP_IMAGE_PATH,
};
#endif
void system_tts_policy_init(void)
{
#ifdef CONFIG_PLAYTTS
	tts_manager_register_tts_config(&user_tts_config);
#endif
}

