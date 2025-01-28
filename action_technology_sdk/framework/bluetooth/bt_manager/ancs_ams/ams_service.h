#ifndef __AMS_SERVICE_H__
#define __AMS_SERVICE_H__

#ifdef _cplusplus
extern "C" {
#endif

#define AMS_STR_LEN 24
#define ENTITY_ATTR_MAX 4

enum remote_cmd_id {
    REMOTE_COMMAND_PLAY = 0,
    REMOTE_COMMAND_PAUSE,
    REMOTE_COMMAND_TOGGLE_PLAY_PAUSE,
    REMOTE_COMMAND_NEXT_TRACK,
    REMOTE_COMMAND_PREIOUS_TRACK,
    REMOTE_COMMAND_VOLUME_UP,
    REMOTE_COMMAND_VOLUME_DOWN,
    REMOTE_COMMAND_ADVANCE_REPEAT_MODE,
    REMOTE_COMMAND_ADVANCE_SHUFFLE_MODE,
    REMOTE_COMMAND_SKIP_FORWARD,
    REMOTE_COMMAND_SKIP_BACKWARD,
    REMOTE_COMMAND_LIKE_TRACK,
    REMOTE_COMMAND_DISLIKE_TRACK,
    REMOTE_COMMAND_BOOKMARK_TRACK,
    REMOTE_COMMAND_NUM,
};

enum entity_id {
    ENTITY_PLAYER = 0,
    ENTITY_QUEUE,
    ENTITY_TRACK,
    ENTITY_NUM,
};

enum player_attr_id {
    PLAYER_ATTR_NAME = 0,
    PLAYER_ATTR_PLAYBACK_INFO,
    PLAYER_ATTR_VOLUME,
};

enum playback_state {
    PLAYBACK_STATE_PAUSED = 0,
    PLAYBACK_STATE_PLAYING,
    PLAYBACK_STATE_REWINDING,
    PLAYBACK_STATE_FAST_FORWARDING,
};

enum queue_attr_id {
    QUEUE_ATTR_INDEX = 0,
    QUEUE_ATTR_COUNT,
    QUEUE_ATTR_SHUFFLE_MODE,
    QUEUE_ATTR_REPEAT_MODE,
};

enum shuffle_mode {
    SHUFFLE_MODE_OFF = 0,
    SHUFFLE_MODE_ONE,
    SHUFFLE_MODE_ALL,
};

enum repeat_mode {
    REPEAT_MODE_OFF = 0,
    REPEAT_MODE_ONE,
    REPEAT_MODE_ALL,
};

enum track_attr_id {
    TRACK_ATTR_ARTIST = 0,
    TRACK_ATTR_ALBUM,
    TRACK_ATTR_TITLE,
    TRACK_ATTR_DURATION,
};

enum entity_update_flags {
    EUF_TRUNCATED = 0,
};

struct entity_update_notify {
    uint8_t entity_id;
    uint8_t attr_id;
    uint8_t flags;
    uint8_t value[0];
};

struct entity_player {
    char name[AMS_STR_LEN];
    struct {
        uint8_t state;
        float rate;
        float elapsed_time;
    } playback;
    float volume;
};

struct entity_queue {
    uint16_t index;
    uint16_t count;
    uint8_t shuffle_mode;
    uint8_t repeat_mode;
};

struct entity_track {
    char artist[AMS_STR_LEN];
    char album[AMS_STR_LEN];
    char title[AMS_STR_LEN];
    float duration;
};

struct entity_attr {
    uint8_t entity_id;
    uint8_t attr_id;
};

extern void ams_service_data_init(void);

int ams_write_remote_command(uint8_t cmd);

extern int ams_write_entity_update(uint8_t entity_id);

int ams_write_entity_attr(uint8_t entity_id, uint8_t attr_id);

int ams_read_entity_attr(void);

int config_ams_remote_cmd_notifications(struct bt_conn *conn,bool enable);

int config_ams_entity_update_notifications(struct bt_conn *conn,bool enable);

#ifdef _cplusplus
}
#endif

#endif  /* __AMS_SERVICE_H__ */
