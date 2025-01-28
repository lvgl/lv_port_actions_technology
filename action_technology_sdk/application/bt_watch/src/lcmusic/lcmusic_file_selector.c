/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief lcmusic app file selector.
 */

#include "lcmusic.h"
#ifdef CONFIG_DVFS
#include <dvfs.h>
#endif

#define RECORD_DIR "RECORD"
#define ALARM_DIR "ALARM"
#define LOOP_DIR  "LOOP"
#define SYSTEM_DIR "System"//"SYSTEM"

#if CONFIG_BKG_SCAN_DISK
#define SCAN_DISK_STACKSIZE	1536
static u8_t scan_disk_stack[SCAN_DISK_STACKSIZE];
#endif

OS_MUTEX_DEFINE(iter_mutex);

iterator_t *iter = NULL;
static int _iterator_filter_dir(const char *path)
{
	/*lcmusic app filter record file, alarm file and loop music file **/
	if (strncmp(app_manager_get_current_app(), APP_ID_UHOST_MPLAYER, strlen(APP_ID_UHOST_MPLAYER)) == 0
		|| strncmp(app_manager_get_current_app(), APP_ID_SD_MPLAYER, strlen(APP_ID_SD_MPLAYER)) == 0) {
		if (strstr(path, RECORD_DIR) || strstr(path, ALARM_DIR) || strstr(path, SYSTEM_DIR) || strstr(path, LOOP_DIR))
			return true;
		else
			return false;
	} else
		return false;
}
static int _iterator_match_fn(const char *path, int is_dir)
{
	if (strstr(path, ALARM_DIR))
		return false;

	if (!is_dir) {
	#ifndef CONFIG_FORMAT_CHECK
		return true;
	#else
		return get_music_file_type(path) != UNSUPPORT_TYPE;
	#endif
	}

	if (_iterator_filter_dir(path))
		return false;
	else
		return true;
}

int lcmusic_init_iterator(struct lcmusic_app_t *lcmusic)
{
	file_iterator_cursor_t cursor;
	file_iterator_param_t param;
	int res = 0;

	cursor.path = lcmusic->cur_url;

	memset(&param, 0, sizeof(param));
	if (lcmusic->cur_url[0] == 0) {
		SYS_LOG_INF("cur_url null\n");
		cursor.path = NULL;
	}
	param.cursor = &cursor;
	param.topdir = lcmusic->cur_dir;
	param.max_level = MAX_DIR_LEVEL;

	param.match_fn = _iterator_match_fn;

	if (iter) {
		SYS_LOG_INF("iter exist\n");
	} else {
		iter = file_iterator_create(&param);
	}

	if (iter)
		iterator_get_plist_info(iter, &lcmusic->sum_track_no);
	else
		res = -ENOSYS;
	return res;
}
void lcmusic_exit_iterator(void)
{
	os_mutex_lock(&iter_mutex, OS_FOREVER);

	if (iter)
		iterator_destroy(iter);
	iter = NULL;
	os_mutex_unlock(&iter_mutex);
}

const char *lcmusic_play_next_url(struct lcmusic_app_t *lcmusic, bool force_switch)
{
	os_mutex_lock(&iter_mutex, OS_FOREVER);

	if (!iter) {
		SYS_LOG_WRN("iter null\n");
		lcmusic->mplayer_state = LCMUSIC_MPLAYER_STATE_ERROR;
		os_mutex_unlock(&iter_mutex);
		return NULL;
	}
	/*manual switch reset restart_iterator_times*/
	if ((lcmusic->music_state != LCMUSIC_STATUS_NULL) && (lcmusic->music_state != LCMUSIC_STATUS_ERROR))
		lcmusic->full_cycle_times = 0;

	const char *path = iterator_next(iter, force_switch, &lcmusic->music_bp_info.track_no);

	if (lcmusic->music_bp_info.track_no == lcmusic->sum_track_no)
		lcmusic->full_cycle_times++;

	if (!path || lcmusic->full_cycle_times > 2) {
		lcmusic->mplayer_state = LCMUSIC_MPLAYER_STATE_ERROR;
		SYS_LOG_WRN("cycle (%d)\n", lcmusic->full_cycle_times);
		os_mutex_unlock(&iter_mutex);
		return NULL;
	}

	strncpy(lcmusic->cur_url, path, MAX_URL_LEN);
	lcmusic->cur_url[sizeof(lcmusic->cur_url) - 1] = 0;
	os_mutex_unlock(&iter_mutex);

	return lcmusic->cur_url;
}
const char *lcmusic_play_prev_url(struct lcmusic_app_t *lcmusic)
{
	os_mutex_lock(&iter_mutex, OS_FOREVER);
	if (!iter) {
		SYS_LOG_WRN("iter null\n");
		lcmusic->mplayer_state = LCMUSIC_MPLAYER_STATE_ERROR;
		os_mutex_unlock(&iter_mutex);
		return NULL;
	}

	/*manual switch reset restart_iterator_times*/
	if ((lcmusic->music_state != LCMUSIC_STATUS_NULL) && (lcmusic->music_state != LCMUSIC_STATUS_ERROR))
		lcmusic->full_cycle_times = 0;

	const char *path = iterator_prev(iter, &lcmusic->music_bp_info.track_no);

	if (lcmusic->music_bp_info.track_no == lcmusic->sum_track_no)
		lcmusic->full_cycle_times++;

	if (!path || lcmusic->full_cycle_times > 2) {
		lcmusic->mplayer_state = LCMUSIC_MPLAYER_STATE_ERROR;
		SYS_LOG_WRN("cycle (%d)\n", lcmusic->full_cycle_times);
		os_mutex_unlock(&iter_mutex);
		return NULL;
	}

	strncpy(lcmusic->cur_url, path, MAX_URL_LEN);
	lcmusic->cur_url[sizeof(lcmusic->cur_url) - 1] = 0;
	os_mutex_unlock(&iter_mutex);
	return lcmusic->cur_url;
}

const char *lcmusic_play_next_folder_url(struct lcmusic_app_t *lcmusic)
{
	os_mutex_lock(&iter_mutex, OS_FOREVER);
	if (!iter) {
		SYS_LOG_WRN("iter null\n");
		lcmusic->mplayer_state = LCMUSIC_MPLAYER_STATE_ERROR;
		os_mutex_unlock(&iter_mutex);
		return NULL;
	}
	const char *path = iterator_next_folder(iter, &lcmusic->music_bp_info.track_no);

	if (!path) {
		SYS_LOG_WRN("path null\n");
		os_mutex_unlock(&iter_mutex);
		return NULL;
	}

	strncpy(lcmusic->cur_url, path, MAX_URL_LEN);
	lcmusic->cur_url[sizeof(lcmusic->cur_url) - 1] = 0;
	os_mutex_unlock(&iter_mutex);
	return lcmusic->cur_url;
}

const char *lcmusic_play_prev_folder_url(struct lcmusic_app_t *lcmusic)
{
	os_mutex_lock(&iter_mutex, OS_FOREVER);
	if (!iter) {
		SYS_LOG_WRN("iter null\n");
		lcmusic->mplayer_state = LCMUSIC_MPLAYER_STATE_ERROR;
		os_mutex_unlock(&iter_mutex);
		return NULL;
	}
	const char *path = iterator_prev_folder(iter, &lcmusic->music_bp_info.track_no);

	if (!path) {
		SYS_LOG_WRN("path null\n");
		lcmusic->mplayer_state = LCMUSIC_MPLAYER_STATE_ERROR;
		os_mutex_unlock(&iter_mutex);
		return NULL;
	}

	strncpy(lcmusic->cur_url, path, MAX_URL_LEN);
	lcmusic->cur_url[sizeof(lcmusic->cur_url) - 1] = 0;
	os_mutex_unlock(&iter_mutex);
	return lcmusic->cur_url;
}

const char *lcmusic_play_set_track_no(struct lcmusic_app_t *lcmusic, u16_t track_no)
{
	os_mutex_lock(&iter_mutex, OS_FOREVER);
	if (!iter) {
		SYS_LOG_WRN("iter null\n");
		lcmusic->mplayer_state = LCMUSIC_MPLAYER_STATE_ERROR;
		os_mutex_unlock(&iter_mutex);
		return NULL;
	}
	const char *path = iterator_set_track_no(iter, track_no);

	if (!path) {
		lcmusic->mplayer_state = LCMUSIC_MPLAYER_STATE_ERROR;
		SYS_LOG_WRN("path null\n");
		os_mutex_unlock(&iter_mutex);
		return NULL;
	}

	strncpy(lcmusic->cur_url, path, MAX_URL_LEN);
	lcmusic->cur_url[sizeof(lcmusic->cur_url) - 1] = 0;
	os_mutex_unlock(&iter_mutex);
	return lcmusic->cur_url;
}

int lcmusic_play_set_mode(struct lcmusic_app_t *lcmusic, u8_t mode)
{
	if (!iter) {
		SYS_LOG_WRN("iter null\n");
		lcmusic->mplayer_state = LCMUSIC_MPLAYER_STATE_ERROR;
		return -ENOSYS;
	}
	return iterator_set_mode(iter, mode);
}

#if CONFIG_SUPPORT_FILE_FULL_NAME
int get_dir_layer(char *url, const char *dir)
{
	int layer = -1;

	if (dir[strlen(dir) - 1] == ':')
		layer = 0;
	do {
		if (*url== '/') {
			layer++;
		}
	} while (*url++);

	return layer;
}
/*url:SD:SNA0~ROOTDI~1/SN40~A.MP3*/
int lcmusic_get_cluster_by_url(struct lcmusic_app_t *lcmusic)
{
	char *temp_url = NULL;
	struct fs_dir_t *zdp = NULL;
	struct fs_dirent *entry = NULL;
	int res = -EINVAL;
	char *str = NULL;
	int dir_layer = 0;
	DIR* dp = NULL;
	u32_t temp_cluster = 0;

	temp_url = app_mem_malloc(strlen(lcmusic->cur_url) + 1);
	zdp = app_mem_malloc(sizeof(struct fs_dir_t));
	entry = app_mem_malloc(sizeof(struct fs_dirent));
	if (!temp_url || !zdp || !entry)
		goto exit;

	strcpy(temp_url, lcmusic->cur_url);
	dir_layer = get_dir_layer(temp_url, lcmusic->cur_dir);
	if (dir_layer >= MAX_DIR_LEVEL || dir_layer < 0) {
		SYS_LOG_WRN("layer(%d) exceed max(%d)\n", dir_layer, MAX_DIR_LEVEL);
		goto exit;
	}
	memset(lcmusic->music_bp_info.file_dir_info, 0, sizeof(lcmusic->music_bp_info.file_dir_info));

	do {
		str = strrchr(temp_url, '/');
		if (!str) {
			if (!strcmp(temp_url, lcmusic->cur_dir))
				goto exit;
			str = strrchr(temp_url, ':');
		}
		if (str) {
			str[0] = 0;
			str++;
			memset(zdp, 0, sizeof(struct fs_dir_t));
			if (!strstr(temp_url, lcmusic->cur_dir)) {
				res = fs_opendir(zdp, lcmusic->cur_dir);
			} else {
				res = fs_opendir(zdp, temp_url);
			}
			if (res) {
				SYS_LOG_WRN("opendir %s failed (res=%d)\n", temp_url, res);
				goto exit;
			} else {
				dp = &zdp->dp;
				temp_cluster = dp->clust;
				do {
					res = fs_readdir(zdp, entry);
					if (res || entry->name[0] == 0) {
						SYS_LOG_ERR("fs_readdir failed (res=%d), skip this\n", res);
						fs_closedir(zdp);
						res = -EINVAL;
						goto exit;
					} else {
						if (strcmp(entry->name,str) == 0) {
							/*take care of cur dir more than one cluster*/
							lcmusic->music_bp_info.file_dir_info[dir_layer].cluster = temp_cluster;
							lcmusic->music_bp_info.file_dir_info[dir_layer].dirent_blk_ofs = dp->blk_ofs;
							if (entry->type == FS_DIR_ENTRY_FILE)
								lcmusic->music_bp_info.file_size = entry->size;
							dir_layer--;
							fs_closedir(zdp);
							break;
						}
					}
				} while (1);
			}
		}
	} while (str);

exit:
	if (temp_url)
		app_mem_free(temp_url);
	if (zdp)
		app_mem_free(zdp);
	if (entry)
		app_mem_free(entry);
	return res;
}

int lcmusic_get_url_by_cluster(struct lcmusic_app_t *lcmusic)
{
	int i = 0;
	int res = -EINVAL;
	struct fs_dir_t *zdp = NULL;
	struct fs_dirent *entry = NULL;
	int dir_layer = 0;
	u32_t file_size = 0;

	zdp = app_mem_malloc(sizeof(struct fs_dir_t));
	entry = app_mem_malloc(sizeof(struct fs_dirent));
	if (!zdp || !entry)
		goto exit;

	for(i = 0; i < MAX_DIR_LEVEL; i++) {
		if (lcmusic->music_bp_info.file_dir_info[i].cluster == 0 &&
			lcmusic->music_bp_info.file_dir_info[i].dirent_blk_ofs == 0) {
			if (i== 0 && lcmusic->music_bp_info.file_size != 0) {
				;
			} else {
				break;
			}
		}
	}

	if (i == 0) {
		SYS_LOG_INF("bp invalid\n");
		memset(&lcmusic->music_bp_info, 0, sizeof(lcmusic->music_bp_info));
		goto exit;
	}
	strcpy(lcmusic->cur_url, lcmusic->cur_dir);
	if (lcmusic->cur_url[strlen(lcmusic->cur_url) - 1] != ':' && lcmusic->cur_url[strlen(lcmusic->cur_url) - 1] != '/')
		lcmusic->cur_url[strlen(lcmusic->cur_url)] = '/';

	dir_layer = --i;
	i = 0;
	do {
		memset(zdp, 0, sizeof(struct fs_dir_t));
		res = fs_opendir_cluster(zdp, lcmusic->cur_dir,
			lcmusic->music_bp_info.file_dir_info[i].cluster,
			lcmusic->music_bp_info.file_dir_info[i].dirent_blk_ofs);

		if (!res) {
			memset(entry, 0, sizeof(struct fs_dirent));
			res = fs_readdir(zdp, entry);
			if (res || entry->name[0] == 0) {
				SYS_LOG_ERR("fs_readdir failed (res=%d), skip this\n", res);
				fs_closedir(zdp);
				res = -EINVAL;
				goto exit;
			}

			if (entry->type == FS_DIR_ENTRY_FILE && _iterator_match_fn(entry->name, 0)) {
				strcpy(lcmusic->cur_url + strlen(lcmusic->cur_url), entry->name);
				lcmusic->cur_url[strlen(lcmusic->cur_url)] = 0;
				SYS_LOG_INF("url:%s\n",lcmusic->cur_url);
				fs_closedir(zdp);
				file_size = entry->size;
				if (dir_layer > i || file_size != lcmusic->music_bp_info.file_size) {
					res = -EINVAL;
					SYS_LOG_INF("bp no match layer(%d),i(%d),file_size(%d)\n", dir_layer, i, file_size);
					goto exit;
				}
			} else if (entry->type == FS_DIR_ENTRY_DIR && _iterator_match_fn(entry->name, 1)) {
				strcpy(lcmusic->cur_url + strlen(lcmusic->cur_url), entry->name);
				lcmusic->cur_url[strlen(lcmusic->cur_url)] = '/';
				SYS_LOG_DBG("dir name:%s\n",lcmusic->cur_url);
				fs_closedir(zdp);
			} else {
				SYS_LOG_WRN("bp no match name:%s,type=%d\n", entry->name,entry->type);
				res = -EINVAL;
				goto exit;
			}
		} else {
			SYS_LOG_ERR("fs_opendir failed (res=%d)\n", res);
			fs_closedir(zdp);
			goto exit;
		}
	} while (dir_layer > i++);

exit :
	if (zdp)
		app_mem_free(zdp);
	if (entry)
		app_mem_free(entry);
	if (res) {
		memset(lcmusic->cur_url, 0,sizeof(lcmusic->cur_url));
		memset(&lcmusic->music_bp_info, 0, sizeof(lcmusic->music_bp_info));
	}
	return res;
}
#else
/*url:bycluster:/SD://cluster:2/320/12992420/.mp3*/
int lcmusic_get_cluster_by_url(struct lcmusic_app_t *lcmusic)
{
	char *str = NULL;
	char *cluster = NULL;
	char *blk_ofs = NULL;
	char *file_size = NULL;
	char *temp_url = NULL;
	int res = -EINVAL;

	temp_url = app_mem_malloc(strlen(lcmusic->cur_url) + 1);
	if (!temp_url)
		goto exit;

	strcpy(temp_url, lcmusic->cur_url);
	str = strstr(temp_url,"/cluster:");
	if (!str)
		goto exit;

	str += strlen("/cluster:");
	cluster = str;
	str = strchr(str, '/');
	if (!str)
		goto exit;
	str[0] = 0;

	str++;
	blk_ofs = str;
	str = strchr(str, '/');
	if (!str)
		goto exit;

	str[0] = 0;
	str++;
	file_size = str;
	str = strchr(str, '/');
	if (str)
		str[0] = 0;

	lcmusic->music_bp_info.file_dir_info[0].cluster = atoi(cluster);
	lcmusic->music_bp_info.file_dir_info[0].dirent_blk_ofs = atoi(blk_ofs);
	lcmusic->music_bp_info.file_size = atoi(file_size);

	SYS_LOG_INF("clust=%u,blk_ofs=%u,file_size=%u\n", lcmusic->music_bp_info.file_dir_info[0].cluster,
		lcmusic->music_bp_info.file_dir_info[0].dirent_blk_ofs, lcmusic->music_bp_info.file_size);
	res = 0;

exit:
	if (temp_url)
		app_mem_free(temp_url);
	return res;

}

#define OPEN_MODE "bycluster:"
#define CLUSTER "cluster:"
static int _lcmusic_validate_bp_info(struct lcmusic_app_t *lcmusic)
{
	int res = -EINVAL;
	struct fs_dir_t *zdp = NULL;
	struct fs_dirent *entry = NULL;
	struct fs_file_t zfp;
	uint32_t size;

	zdp = app_mem_malloc(sizeof(struct fs_dir_t));
	entry = app_mem_malloc(sizeof(struct fs_dirent));
	if (!zdp || !entry)
		goto exit;

	if (lcmusic->music_bp_info.file_dir_info[0].cluster ||
		lcmusic->music_bp_info.file_dir_info[0].dirent_blk_ofs ||
		lcmusic->music_bp_info.file_size) {
		//check file exists, not that the exact file, but to check whether card cleard
		memset(&zfp, 0, sizeof(struct fs_file_t));
		res = fs_open_cluster(&zfp, lcmusic->cur_dir, lcmusic->music_bp_info.file_dir_info[0].cluster,
				lcmusic->music_bp_info.file_dir_info[0].dirent_blk_ofs);
		if(res != 0)
		{
			SYS_LOG_ERR("bp open file invalid (res=%d)\n ", res);
			res = -EINVAL;
			goto exit;
		}
		res = fs_seek(&zfp, 0, FS_SEEK_END);
		if(res != 0)
		{
			SYS_LOG_ERR("bp seek file invalid (res=%d)\n ", res);
			fs_close(&zfp);
			res = -EINVAL;
			goto exit;
		}
		size = fs_tell(&zfp);
		if(size == 0)
		{
			SYS_LOG_ERR("bp check file invalid (res=%d)\n ", res);
			fs_close(&zfp);
			res = -EINVAL;
			goto exit;
		}
		fs_close(&zfp);

		/*check the bp is valid*/
		res = fs_opendir_cluster(zdp, lcmusic->cur_dir,
				lcmusic->music_bp_info.file_dir_info[0].cluster,
				lcmusic->music_bp_info.file_dir_info[0].dirent_blk_ofs);
		if (!res) {
			res = fs_readdir(zdp, entry);
			if (res || entry->name[0] == 0) {
				SYS_LOG_ERR("bp invalid (res=%d)\n", res);
				fs_closedir(zdp);
				res = -EINVAL;
				goto exit;
			}

			if (entry->type != FS_DIR_ENTRY_FILE || !_iterator_match_fn(entry->name, 0)
				|| (u32_t)(entry->size) != lcmusic->music_bp_info.file_size) {
				res = -EINVAL;
				SYS_LOG_ERR("bp unmatch\n");
			}
			SYS_LOG_INF("bp file:%s\n",entry->name);

			/*get file path*/
			memset(lcmusic->cur_url, 0, sizeof(lcmusic->cur_url));
			snprintf(lcmusic->cur_url, sizeof(lcmusic->cur_url), "%s%s/%s%u/%u/%u/%s",
				OPEN_MODE, lcmusic->cur_dir, CLUSTER, lcmusic->music_bp_info.file_dir_info[0].cluster,
				lcmusic->music_bp_info.file_dir_info[0].dirent_blk_ofs, lcmusic->music_bp_info.file_size, entry->name);

			fs_closedir(zdp);
		}
	}

exit:
	if (zdp)
		app_mem_free(zdp);
	if (entry)
		app_mem_free(entry);
	return res;
}

/* url:bycluster:/SD://cluster:2/2144/8205477/8607_128kbps_44100_2_汪峰 - 再见 二十世纪.mp3*/
int lcmusic_get_url_by_cluster(struct lcmusic_app_t *lcmusic)
{
	int res = -EINVAL;

	if (!_lcmusic_validate_bp_info(lcmusic)) {
		res = 0;
	} else {
		memset(lcmusic->cur_url, 0,sizeof(lcmusic->cur_url));
		memset(&lcmusic->music_bp_info, 0, sizeof(lcmusic->music_bp_info));
	}
	return res;
}
#endif

static void _lcmusic_scan_disk_start(void)
{
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();

	if (!lcmusic)
		return;

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "scan_disk");
#endif
	lcmusic_init_iterator(lcmusic);
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "scan_disk");
#endif

}
#if CONFIG_BKG_SCAN_DISK
static void _lcmusic_scan_disk_thread(void *parama1, void *parama2, void *parama3)
{
	struct lcmusic_app_t *lcmusic = lcmusic_get_app();

	if (!lcmusic)
		return;
	os_mutex_lock(&iter_mutex, OS_FOREVER);
	if (lcmusic->cur_url[0] != 0) {
		os_sleep(500);
	}
	lcmusic->disk_scanning = 1;
	_lcmusic_scan_disk_start();
	lcmusic->disk_scanning = 0;
	os_mutex_unlock(&iter_mutex);
}
#endif

void lcmusic_scan_disk(void)
{
#if CONFIG_BKG_SCAN_DISK
/* create a thread to scan disk*/
	os_thread_create(scan_disk_stack, SCAN_DISK_STACKSIZE,
		_lcmusic_scan_disk_thread,
		NULL, NULL, NULL,
		CONFIG_APP_PRIORITY - 1, 0, 0);
#else
	_lcmusic_scan_disk_start();
#endif
}

