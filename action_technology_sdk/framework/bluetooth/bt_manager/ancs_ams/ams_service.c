/**
 * @file amp.c
 * @brief Apple Media Profile
 */

/**
 * Copyright (c) 2021 Actions Semiconductor Co., Ltd
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <shell/shell.h>

#include <acts_bluetooth/bluetooth.h>
#include <acts_bluetooth/conn.h>
#include <acts_bluetooth/gatt.h>
#include <acts_bluetooth/uuid.h>
#include <acts_bluetooth/host_interface.h>

//#include "app_defines.h"
#include "ble_ancs_ams.h"
#include <msg_manager.h>
#include "ams_service.h"
#include "ams_service_disc.h"
#include "ancs_ams_log.h"

static const char *remote_command_str[] = {
    "play", "pause", "toggle play pause", "next track", "previous track", "volume up",
    "volume down", "advance repeate mode", "advance shuffle mode", "skip forward",
    "skip backward", "like track", "dislike track", "bookmark track"
};

static const char *playback_state_str[] = {
    "paused", "playing", "rewinding", "fast forwarding"
};

static const char *mode_str[] = {
    "off", "one", "all"
};

//static struct bt_conn *def_conn;

/* gatt params */
static uint8_t ams_read_pending_flag = 0;
static uint8_t ams_write_pending_flag = 0;
static bool ams_gatt_subscribe_entity_update = 0;
static bool ams_gatt_subscribe_entity_init = 0;
static bool ams_gatt_subscribe_remote_cmd = 0;
static struct bt_gatt_read_params ams_gatt_read_params;
static struct bt_gatt_write_params ams_gatt_write_params;
static struct bt_gatt_subscribe_params subscribe_params_rc;
static struct bt_gatt_subscribe_params subscribe_params_eu;

/* ams */
ATOMIC_DEFINE(support_commands, REMOTE_COMMAND_NUM);
static struct entity_player player;
static struct entity_queue queue;
static struct entity_track track;
/* current attribute of entity to read */
static struct entity_attr read_attr;
//static uint8_t write_hold;
static double pow(double x, double y)
{
	double result = 1;

	if (y < 0) {
		y = -y;
		while (y--) {
			result /= x;
		}
	} else {
		while (y--) {
			result *= x;
		}
	}

	return result;
}

static float __atof(const char *s)
{
    int n = 0;
    float f = 0;
    int i = 1;

    while (isdigit((int)*s)) {
        n = 10 *n + (*s++ - '0');
    }

    if (*s != '.') {
        return (float)n;
    }

    s++;
    while (isdigit((int)*s)) {
        f += (*s++ - '0') * pow(0.1, i++);
    }

    return (float)n + f;
}

static void write_func(struct bt_conn *conn, uint8_t err,
	struct bt_gatt_write_params *params)
{
    ams_write_pending_flag = 0;
    ams_log_inf("ams gatt_write complete handle:%x %d\n",params->handle,params->length);
    if(params->handle == get_ams_entity_update_handle()){
        uint8_t * cmd = (uint8_t *)params->data;
        if(cmd[0] == ENTITY_PLAYER){
            ble_ancs_ams_send_msg_to_app(BLE_AMS_EVENT_WRITE_ENTITY_UPDATE,ENTITY_PLAYER);
        }
    }
}

static int gatt_write(void *data, uint16_t len, uint16_t handle)
{
    int err;
	struct bt_conn *conn = ble_get_ancs_ams()->pconn;
    if(!conn){
        return -EINVAL;
    }		

    if(ams_write_pending_flag){
        ams_log_err("ams write busy! %x",handle);
        return -EBUSY;
    }

    memset(&ams_gatt_write_params, 0, sizeof(struct bt_gatt_write_params));

    ams_gatt_write_params.data = data;
    ams_gatt_write_params.length = len;
    ams_gatt_write_params.handle = handle;
    ams_gatt_write_params.offset = 0;
    ams_gatt_write_params.func = write_func;

    err = hostif_bt_gatt_write(conn, &ams_gatt_write_params);
    if (err) {
        ams_log_err("write failed(err %d)", err);
    }
    else{
        ams_write_pending_flag = 1;
        ams_log_inf("ams gatt_write:%x\n",ams_gatt_write_params.handle);
    }
    return err;
}

int ams_write_remote_command(uint8_t cmd)
{
    if (!atomic_test_bit(subscribe_params_rc.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE)) {
        return -EIO;
    }

    if (cmd >= REMOTE_COMMAND_NUM) {
        return -EINVAL;
    }

    if (!atomic_test_bit(support_commands, cmd)) {
        ams_log_err("Unsupport command: %d", cmd);
        return -EINVAL;
    }

    return gatt_write(&cmd, 1, get_ams_remote_cmd_handle());
}

extern int ams_write_entity_update(uint8_t entity_id)
{
    uint8_t len = 0;
    uint8_t cmd[5] = {0};

    if (!atomic_test_bit(subscribe_params_eu.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE)) {
        return -EIO;
    }

    cmd[len++] = entity_id;

    switch (entity_id) {
    case ENTITY_PLAYER:
        cmd[len++] = PLAYER_ATTR_NAME;
        cmd[len++] = PLAYER_ATTR_PLAYBACK_INFO;
        cmd[len++] = PLAYER_ATTR_VOLUME;
        break;
    case ENTITY_QUEUE:
        cmd[len++] = QUEUE_ATTR_INDEX;
        cmd[len++] = QUEUE_ATTR_COUNT;
        cmd[len++] = QUEUE_ATTR_SHUFFLE_MODE;
        cmd[len++] = QUEUE_ATTR_REPEAT_MODE;
        break;
    case ENTITY_TRACK:
        cmd[len++] = TRACK_ATTR_ARTIST;
        cmd[len++] = TRACK_ATTR_ALBUM;
        cmd[len++] = TRACK_ATTR_TITLE;
        cmd[len++] = TRACK_ATTR_DURATION;
        break;
    default:
        ams_log_err("Unknown entity id %d", entity_id);
        return -EINVAL;
    }

    return gatt_write(cmd, len, get_ams_entity_update_handle());
}

static uint8_t read_func(struct bt_conn *conn, uint8_t err,
	struct bt_gatt_read_params *params,
	const void *data, uint16_t length)
{
    char *str;

    ams_read_pending_flag = 0;

    if (!data) {
        return BT_GATT_ITER_STOP;
    }

    if (length) {
        str = (char *)data;
        ble_ancs_ams_send_msg_to_app(BLE_AMS_EVENT_READ_ENTITY_UPDATE,params->single.handle);
    }
    return 0;
}

int ams_write_entity_attr(uint8_t entity_id, uint8_t attr_id)
{
    read_attr.entity_id = entity_id;
    read_attr.attr_id = attr_id;

    return gatt_write(&read_attr, sizeof(read_attr), get_ams_entity_attr_handle());
}

int ams_read_entity_attr(void)
{
    int err;
	struct bt_conn *conn = ble_get_ancs_ams()->pconn;
    if(!conn){
        return -EINVAL;
    }	

    if(ams_read_pending_flag){
        return -EBUSY;
    }

    memset(&ams_gatt_read_params, 0, sizeof(struct bt_gatt_read_params));

    ams_gatt_read_params.func = read_func;
    ams_gatt_read_params.handle_count = 1;
    ams_gatt_read_params.single.handle = get_ams_entity_attr_handle();
    ams_gatt_read_params.single.offset = 0;

    err = hostif_bt_gatt_read(conn, &ams_gatt_read_params);
    if (err) {
        ams_log_err("Read entity attr failed(err %d)\n", err);
    } 
    else{
        ams_read_pending_flag = 1;
    }
    return err;
}

/* send a message to main thread */
static void remote_command_handle(const uint8_t *data, uint16_t len)
{
    uint8_t i, cmd_id;

    /* clear all bits first */
    atomic_clear(support_commands);

    for (i = 0; i < len; i++) {
        cmd_id = data[i];
        ams_log_inf("[Support command] %s(%d)\n", remote_command_str[cmd_id], cmd_id);
        atomic_set_bit(support_commands, data[i]);
    }
}

void ams_show_info(uint8_t entity_id)
{
    switch (entity_id) {
    case ENTITY_PLAYER:
        ams_log_inf("[Player]\n");
        ams_log_inf("name: %s\n", player.name);
        ams_log_inf("playback state: %s\n", playback_state_str[player.playback.state]);
        ams_log_inf("playback rate: %f\n", player.playback.rate);
        ams_log_inf("playback elapsed time: %f\n", player.playback.elapsed_time);
        ams_log_inf("volume: %f\n", player.volume);
        break;
    case ENTITY_QUEUE:
        ams_log_inf("[Queue]\n");
        ams_log_inf("index: %d\n", queue.index);
        ams_log_inf("count: %d\n", queue.count);
        ams_log_inf("shuffle mode: %s\n", mode_str[queue.shuffle_mode]);
        ams_log_inf("repeat mode: %s\n", mode_str[queue.repeat_mode]);
        break;
    case ENTITY_TRACK:
        ams_log_inf("[Track]\n");
        ams_log_inf("artist: %s\n", track.artist);
        ams_log_inf("album: %s\n", track.album);
        ams_log_inf("title: %s\n", track.title);
        ams_log_inf("duration: %f\n", track.duration);
        break;
    }
}

static void set_playback(uint8_t *value, uint16_t len)
{
    uint16_t i;
    uint8_t comma = 0;

    player.playback.state = atoi(value);

    for (i = 0; i < len; i++) {
        if (value[i] == ',') {
            comma++;
            if (comma == 1) {
                player.playback.rate = __atof(&value[i+1]);
            } else if (comma == 2) {
                player.playback.elapsed_time = __atof(&value[i+1]);
            }
        }
    }
}

static void set_player(uint8_t attr_id, uint8_t *value, uint16_t len)
{
    uint16_t cpy_len;
    
    switch (attr_id) {
    case PLAYER_ATTR_NAME:
        cpy_len = MIN(AMS_STR_LEN-1, len);
        memcpy(player.name, value, cpy_len);
        player.name[cpy_len] = '\0';
        ams_log_inf("[Player] name: %s\n", player.name);
        break;
    case PLAYER_ATTR_PLAYBACK_INFO:
        set_playback(value, len);
        ams_log_inf("[Player] playback state: %s, rate: %.2f, elapsed time: %.2fs\n",
            playback_state_str[player.playback.state], player.playback.rate, player.playback.elapsed_time);
        ble_ancs_ams_notify_playback_pos(player.playback.elapsed_time);
        break;
    case PLAYER_ATTR_VOLUME:
        player.volume = __atof(value);
        ams_log_inf("[Player] volume: %.2f%%\n", player.volume * 100.0);
        break;
    default:
        break;
    }
}

static void set_queue(uint8_t attr_id, uint8_t *value, uint16_t len)
{
    switch (attr_id) {
    case QUEUE_ATTR_INDEX:
        queue.index = atoi(value);
        ams_log_inf("[Queue] index: %d\n", queue.index);
        break;
    case QUEUE_ATTR_COUNT:
        queue.count = atoi(value);
        ams_log_inf("[Queue] count: %d\n", queue.count);
        break;
    case QUEUE_ATTR_SHUFFLE_MODE:
        queue.shuffle_mode = atoi(value);
        ams_log_inf("[Queue] shuffle mode: %s\n", mode_str[queue.shuffle_mode]);
        break;
    case QUEUE_ATTR_REPEAT_MODE:
        queue.repeat_mode = atoi(value);
        ams_log_inf("[Queue] repeat mode: %s\n", mode_str[queue.repeat_mode]);
        break;
    }
}

static void set_track(uint8_t attr_id, uint8_t *value, uint16_t len)
{
    uint16_t cpy_len = (MIN(AMS_STR_LEN-1, len));

    switch (attr_id) {
    case TRACK_ATTR_ARTIST:
        memcpy(track.artist, value, cpy_len);
        track.artist[cpy_len] = '\0';
        ams_log_inf("[Track] artist: %s\n", track.artist);
        break;
    case TRACK_ATTR_ALBUM:
        memcpy(track.album, value, cpy_len);
        track.album[cpy_len] = '\0';
        ams_log_inf("album: %s\n",track.album);
        break;
    case TRACK_ATTR_TITLE:
        memcpy(track.title, value, cpy_len);
        track.title[cpy_len] = '\0';
        ams_log_inf("title: %s\n", track.title);
        break;
    case TRACK_ATTR_DURATION:
        track.duration = __atof(value);
        ams_log_inf("duration: %.2fs\n", track.duration);
        break;
    }  
}

/* update one attribute of entity each time */
static void entity_update_handle(const uint8_t *data, uint16_t len)
{
    uint16_t value_len = len - sizeof(struct entity_update_notify);
    struct entity_update_notify *notify = (void *)data;

    switch (notify->entity_id) {
    case ENTITY_PLAYER:
        set_player(notify->attr_id, notify->value, value_len);
        break;
    case ENTITY_QUEUE:
        set_queue(notify->attr_id, notify->value, value_len);
        break;
    case ENTITY_TRACK:
        set_track(notify->attr_id, notify->value, value_len);
        break;
    default:
        break;
    }
    ble_ancs_ams_send_msg_to_app(BLE_AMS_EVENT_WRITE_ENTITY_UPDATE,notify->entity_id);
}

static uint8_t notify_func(struct bt_conn *conn,
	struct bt_gatt_subscribe_params *params,
	const void *data, uint16_t length)
{
    if (!data) {
        return BT_GATT_ITER_STOP;
    }

    if (params->ccc_handle == get_ams_remote_cmd_ccd_handle()) {
        remote_command_handle(data, length);
    } 
	else if (params->ccc_handle == get_ams_entity_update_ccd_handle()) {
        entity_update_handle(data,length);

        if (1 == ams_gatt_subscribe_entity_init) {
            ams_write_entity_update(ENTITY_TRACK);
            ams_gatt_subscribe_entity_init = 0;
        }
    }

    return BT_GATT_ITER_CONTINUE;
}

int config_ams_remote_cmd_notifications(struct bt_conn *conn,bool enable)
{
    int err;
	subscribe_params_rc.notify = notify_func;
	subscribe_params_rc.value = BT_GATT_CCC_NOTIFY;
	subscribe_params_rc.ccc_handle = get_ams_remote_cmd_ccd_handle();
	subscribe_params_rc.value_handle = get_ams_remote_cmd_handle();
	atomic_set_bit(subscribe_params_rc.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);
	ams_log_inf("subscribe AMS RC handle 0x%2x, 0x%2x %d\n", subscribe_params_rc.ccc_handle, subscribe_params_rc.value_handle, enable);
    if(enable){
        err = hostif_bt_gatt_subscribe(conn, &subscribe_params_rc);
        if (err && err != -EALREADY) {
            ams_log_err("AMS RC Subscribe failed (err %d)\n", err);
        }
        else {
            ams_gatt_subscribe_remote_cmd = 1;
            ams_log_inf("AMS RC subscribe success\n");
        }
    }
    else{
        err = hostif_bt_gatt_unsubscribe(conn, &subscribe_params_rc);
        if (err) {
            ams_log_err("AMS RC unsubscribe failed (err %d)\n", err);
        }
        else {
            ams_gatt_subscribe_remote_cmd = 0;
            ams_log_inf("AMS RC unsubscribe success\n");
        }
    }
	return err;
}

static void ams_write_entity_update_ccd_func(struct bt_conn *conn, uint8_t err,
	struct bt_gatt_write_params *params)
{
    ams_log_inf("AMS write ccc err:%d\n", err);
    if(ams_gatt_subscribe_entity_update == 1){
        ams_write_entity_update(ENTITY_PLAYER);
        ams_gatt_subscribe_entity_init = 1;
    }
}
int config_ams_entity_update_notifications(struct bt_conn *conn,bool enable)
{
    int err;
    subscribe_params_eu.write = ams_write_entity_update_ccd_func;
	subscribe_params_eu.notify = notify_func;
	subscribe_params_eu.value = BT_GATT_CCC_NOTIFY;
	subscribe_params_eu.ccc_handle = get_ams_entity_update_ccd_handle();
	subscribe_params_eu.value_handle = get_ams_entity_update_handle();
	atomic_set_bit(subscribe_params_eu.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);
	ams_log_inf("subscribe AMS EU handle 0x%2x, 0x%2x %d\n", subscribe_params_eu.ccc_handle, subscribe_params_eu.value_handle, enable);
    if(enable){
        err = hostif_bt_gatt_subscribe(conn, &subscribe_params_eu);
        if (err && err != -EALREADY) {
            ams_log_err("AMS EU subscribe failed (err %d)\n", err);
        }
        else {
            ams_gatt_subscribe_entity_update = 1;
            ams_log_err("AMS EU subscribe success\n");
        }
	}
    else{
        err = hostif_bt_gatt_unsubscribe(conn, &subscribe_params_eu);
        if (err) {
            ams_log_err("AMS EU unsubscribe failed (err %d)\n", err);
        }
        else {
            ams_gatt_subscribe_entity_update = 0;
            ams_log_err("AMS EU unsubscribe success\n");
        }
    }
	return err;
}

/* shell command start */
static int cmd_write_remote_cmd(const struct shell *shell,
	size_t argc, char **argv)
{
    return ams_write_remote_command(atoi(argv[1]));
}

extern void ams_service_data_init(void)
{
    ams_read_pending_flag = 0;
    ams_write_pending_flag = 0;
    ams_gatt_subscribe_entity_update = 0;
    ams_gatt_subscribe_entity_init = 0;
    ams_gatt_subscribe_remote_cmd = 0;
}

static int cmd_write_entity_update(const struct shell *shell,
	size_t argc, char **argv)
{
    return ams_write_entity_update(atoi(argv[1]));
}

static int cmd_write_entity_attr(const struct shell *shell,
	size_t argc, char **argv)
{
    return ams_write_entity_attr(atoi(argv[1]), atoi(argv[2]));
}

static int cmd_read_entity_attr(const struct shell *shell,
	size_t argc, char **argv)
{
    return ams_read_entity_attr();
}

static int cmd_show_info(const struct shell *shell,
	size_t argc, char **argv)
{
    ams_show_info(atoi(argv[1]));
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_amp,
    SHELL_CMD(wr-rc, NULL, "write remote command" , cmd_write_remote_cmd),
    SHELL_CMD(wr-eu, NULL, "write entity update [0-2]" , cmd_write_entity_update),
    SHELL_CMD(wr-ea, NULL, "write entity attribute" , cmd_write_entity_attr),
    SHELL_CMD(rd-ea, NULL, "read entity attribute", cmd_read_entity_attr),
    SHELL_CMD(show, NULL, "show entity info", cmd_show_info),
    SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(amp, &sub_amp, "amp commands", NULL);
/* shell command end */
