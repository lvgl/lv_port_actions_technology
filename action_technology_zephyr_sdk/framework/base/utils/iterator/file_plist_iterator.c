#include <zephyr.h>
#include <fs/fs.h>
#include <disk/disk_access.h>
#include <mem_manager.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <iterator/file_iterator.h>
#include <fs_manager.h>
#include <stdio.h>
#include <stdlib.h>
#include <os_common_api.h>

#define MAX_DIR_LEVEL 9
#define FULL_PATH_LEN (MAX_URL_LEN + 2)
#define OPEN_MODE "bycluster:"
#define CLUSTER "cluster:"
#define MAX_SUPPORT_FILE_CNT 10000
#define FAT16_FAR_CLUST 0xfffffff1

struct  folder_info_t {
#if CONFIG_SUPPORT_FILE_FULL_NAME
	uint32_t far_cluster;		/*exfat direntry no farthor cluster*/
	uint32_t blk_ofs;			/*Offset of current entry block being processed*/
#endif
	uint32_t cur_cluster;		/*Current cluster*/
	uint16_t dir_file_count;	/*valid file count*/
	uint8_t dir_layer;	/*curent dir in disk layer */
};
struct play_list_t {
	struct  folder_info_t *folder_info[CONFIG_PLIST_SUPPORT_FOLDER_CNT];
	uint16_t sum_file_count;	/*sum valid files in disk*/
	uint16_t file_seq_num;		/*file in playlist sequence number*/
	uint8_t sum_folder_count;	/*sum valid folder in disk,start from 0*/
	uint8_t folder_seq_num;	/*start from 0*/
	uint16_t dir_file_seq_num;	/*curent file in folder sequence number*/
	uint8_t fs_type;	/*fs type:1--fat12/fat16,0--fat32/exfat */
	uint8_t mode;				/*play mode 0--Full cycle;1--single cycle;2--full,no cycle;3--folder cycle*/
	uint16_t csize;			/* Cluster size [sectors] */
	const char *topdir;
	int (*match_fn)(const char *path, int is_dir);
};

static struct play_list_t *play_list = NULL;

struct file_iterator_data {
	uint8_t has_query_next : 1; /* indicate whether has_next has been called */
	uint8_t has_query_prev : 1; /* indicate whether has_prev has been called */

	/* cached dir structures */
	int16_t level;
	uint16_t max_level;
	struct fs_dir_t *dirs[MAX_DIR_LEVEL];

	/* used to compose new full path */
	uint16_t *dname_len; /* used to store name len of every level directories */
	uint16_t fname_len;  /* name len of current file */
	uint16_t full_len;   /* name len of full path */
	char *full_path;  /* store the full path */

	struct file_iterator_cursor cursor;

	int (*match_fn)(const char *path, int is_dir);

	struct fs_dirent *dirent;
};

static struct play_list_t *get_play_list(void)
{
	return play_list;
}

static int file_iterator_get_plist_info(struct iterator *iter, void *param)
{
	*(uint16_t *)param = play_list->sum_file_count;
	return 0;
}

static int file_iterator_destroy(struct iterator *iter)
{
	struct file_iterator_data *data = iter->data;
	int i;

	if (data->dirent)
		mem_free(data->dirent);

	for (i = 0; i < data->max_level; i++) {
		if (data->dirs[i]) {
			mem_free(data->dirs[i]);
		}
	}
	if (data->dname_len)
		mem_free(data->dname_len);

	if (data->full_path)
		mem_free(data->full_path);

	mem_free(data);

	if (play_list) {
		for (i = 0; i < CONFIG_PLIST_SUPPORT_FOLDER_CNT; i++) {
			if (play_list->folder_info[i]) {
				mem_free(play_list->folder_info[i]);
				play_list->folder_info[i] = NULL;
			}
		}
		mem_free(play_list);
		play_list = NULL;
	}
	return 0;
}

static void calc_track_no_playlist_info(struct play_list_t *plist, uint16_t track_no)
{
	int i = 0;
	uint32_t sum_file = 0;

	if (track_no == 0 || track_no > plist->sum_file_count)
		return;

	plist->file_seq_num = track_no;

	/*calc dir_file_seq_num and folder_seq_num by file_seq_num*/
	for (i = 0; i <= plist->sum_folder_count; i++) {
		sum_file += plist->folder_info[i]->dir_file_count;
		if (sum_file >= plist->file_seq_num) {
			plist->dir_file_seq_num = plist->file_seq_num - (sum_file - plist->folder_info[i]->dir_file_count);
			break;
		}
	}

	if (i <= plist->sum_folder_count) {
		plist->folder_seq_num = i;
		SYS_LOG_INF("file seq num=%d,folder_seq_num=%d,cur_cluster=%d\n",
			plist->file_seq_num, plist->folder_seq_num,
			plist->folder_info[plist->folder_seq_num]->cur_cluster);
	}
}

static void calc_next_playlist_info(struct play_list_t *plist, uint8_t add)
{
	int i = 0;
	uint32_t sum_file = 0;

	if (add) {
		if (plist->file_seq_num < plist->sum_file_count)
			plist->file_seq_num++;
		else
			plist->file_seq_num = 1;
	} else {
		if (plist->file_seq_num > 1)
			plist->file_seq_num--;
		else
			plist->file_seq_num = plist->sum_file_count;
	}
	/*calc dir_file_seq_num and folder_seq_num by file_seq_num*/
	for (i = 0; i <= plist->sum_folder_count; i++) {
		sum_file += plist->folder_info[i]->dir_file_count;
		if (sum_file >= plist->file_seq_num) {
			plist->dir_file_seq_num = plist->file_seq_num - (sum_file - plist->folder_info[i]->dir_file_count);
			break;
		}
	}

	if (i <= plist->sum_folder_count) {
		plist->folder_seq_num = i;
		SYS_LOG_INF("file seq num=%d,folder_seq_num=%d,cur_cluster=%d\n",
			plist->file_seq_num, plist->folder_seq_num,
			plist->folder_info[plist->folder_seq_num]->cur_cluster);
	#if CONFIG_SUPPORT_FILE_FULL_NAME
		SYS_LOG_INF("far_cluster=%d,blk_ofs=%d\n",
			plist->folder_info[i]->far_cluster, plist->folder_info[i]->blk_ofs);
	#endif
	}
}

static void calc_next_folder_playlist_info(struct play_list_t *plist, uint8_t add)
{
	int i = 0;
	uint32_t sum_file = 0;

	if (add) {
		plist->file_seq_num = plist->file_seq_num - plist->dir_file_seq_num
			+ plist->folder_info[plist->folder_seq_num]->dir_file_count + 1;/*get next folder the 1st file*/

		if (plist->file_seq_num > plist->sum_file_count) {/*cur folder is the 1ast one*/
			plist->file_seq_num = 1;
		}
	} else {
		plist->file_seq_num -= plist->dir_file_seq_num;

		if (plist->file_seq_num > 0) {
			while (plist->folder_info[--plist->folder_seq_num]->dir_file_count == 0);/*get prev valid folder*/
			plist->file_seq_num -= plist->folder_info[plist->folder_seq_num]->dir_file_count;
			plist->file_seq_num++;/*start at the lst file of prev valid folder*/
		} else {/*cur folder is the first one*/
			plist->folder_seq_num = plist->sum_folder_count;
			if (plist->folder_info[plist->folder_seq_num]->dir_file_count == 0)
				while (plist->folder_info[--plist->folder_seq_num]->dir_file_count == 0);/*get prev valid folder*/

			plist->file_seq_num = plist->sum_file_count - plist->folder_info[plist->folder_seq_num]->dir_file_count;
			plist->file_seq_num++;/*start at the lst file of prev valid folder*/
		}
	}
	/*calc dir_file_seq_num and folder_seq_num by file_seq_num*/
	for (i = 0; i <= plist->sum_folder_count; i++) {
		sum_file += plist->folder_info[i]->dir_file_count;
		if (sum_file >= plist->file_seq_num) {
			plist->dir_file_seq_num = plist->file_seq_num - (sum_file - plist->folder_info[i]->dir_file_count);
			break;
		}
	}
	/*filter invalidate bp*/
	if (i <= plist->sum_folder_count) {
		plist->folder_seq_num = i;
		SYS_LOG_INF("file seq num=%d,folder_seq_num=%d,cur_cluster=%u\n",
			plist->file_seq_num, plist->folder_seq_num,
			plist->folder_info[plist->folder_seq_num]->cur_cluster);
	}
}
//source:SN60~B.MP3/SNA0~ROOTDI~1/,dest=SD:
//SD:SNA0~ROOTDI~1/SN60~B.MP3
#if CONFIG_SUPPORT_FILE_FULL_NAME
static void get_pos_seq_file_path(char *source, char *dest)
{
	char *buf = NULL;
	//SYS_LOG_INF("source:%s,dest=%s\n", source, dest);

	if (source[strlen(source) - 1] == '/')
		source[strlen(source) - 1] = 0;

	do {
		buf = strrchr(source, '/');
		if (buf) {
			buf[0] = 0;
			strcpy(dest + strlen(dest), ++buf);
			strcpy(dest + strlen(dest), "/");
		} else {
			strcpy(dest + strlen(dest), source);
		}
	} while (buf);
}

static int file_dirname_get(struct play_list_t *plist, struct iterator *iter)
{
	struct file_iterator_data *data = iter->data;
	int res = -ENOENT;
	uint16_t times = 0;
	uint8_t i = 0;
	uint8_t temp_folder_seq = 0;
	char *path_buff = mem_malloc(FULL_PATH_LEN);
	struct fs_dir_t *zdp = mem_malloc(sizeof(struct fs_dir_t));
	struct fs_dirent *entry = mem_malloc(sizeof(struct fs_dirent));

	if (!path_buff || !zdp || !entry || !plist->dir_file_seq_num)
		goto exit;

	/*read file name*/
	res = fs_opendir_cluster(zdp, plist->topdir, plist->folder_info[plist->folder_seq_num]->cur_cluster, 0);
	if (!res) {
		do {
			memset(entry, 0, sizeof(struct fs_dirent));
			res = fs_readdir(zdp, entry);
			if (res || entry->name[0] == 0) {
				SYS_LOG_ERR("fs_readdir failed (res=%d), skip this\n", res);
				fs_closedir(zdp);
				goto exit;
			}
			/* filter out unmatch directory or file */
			if (plist->match_fn && entry->type == FS_DIR_ENTRY_FILE && plist->match_fn(entry->name, 0))
				times++;
		} while (times < plist->dir_file_seq_num);
		strcpy(path_buff + strlen(path_buff), entry->name);
		path_buff[strlen(path_buff)] = '/';
		SYS_LOG_DBG("file name:%s\n",path_buff);
		fs_closedir(zdp);
	} else {
		SYS_LOG_ERR("fs_opendir failed (res=%d)\n", res);
		fs_closedir(zdp);
		goto exit;
	}

	/*gets all parent directory names*/
	temp_folder_seq = plist->folder_seq_num;
	do {
		for(i = 0; i <= plist->sum_folder_count; i++) {
			if (plist->folder_info[temp_folder_seq]->far_cluster == plist->folder_info[i]->cur_cluster) {
				break;
			}
		}

		if (i <= plist->sum_folder_count) {
			memset(zdp, 0, sizeof(struct fs_dir_t));

			res = fs_opendir_cluster(zdp, plist->topdir,
				plist->folder_info[i]->cur_cluster,
				plist->folder_info[temp_folder_seq]->blk_ofs);

			if (!res) {
				temp_folder_seq = i;/*update folder_seq_num*/
				memset(entry, 0, sizeof(struct fs_dirent));
				res = fs_readdir(zdp, entry);
				if (res || entry->name[0] == 0) {
					SYS_LOG_ERR("fs_readdir failed (res=%d), skip this\n", res);
					fs_closedir(zdp);
					goto exit;
				}
				if (plist->match_fn && entry->type == FS_DIR_ENTRY_DIR && plist->match_fn(entry->name, 1)) {
					strcpy(path_buff + strlen(path_buff), entry->name);
					path_buff[strlen(path_buff)] = '/';
					SYS_LOG_DBG("dir name:%s\n",path_buff);
					fs_closedir(zdp);
				} else {
					SYS_LOG_ERR("name:%s,type=%d\n", entry->name, entry->type);
					fs_closedir(zdp);
					goto exit;
				}
			} else {
				SYS_LOG_ERR("fs_opendir failed (res=%d)\n", res);
				fs_closedir(zdp);
				goto exit;
			}
		} else {
			//need todo something
			SYS_LOG_INF("far cluster(%u)\n", plist->folder_info[temp_folder_seq]->far_cluster);
			break;
		}
	} while ((plist->fs_type == 0 && plist->folder_info[temp_folder_seq]->far_cluster) ||
		(plist->fs_type && plist->folder_info[temp_folder_seq]->far_cluster != FAT16_FAR_CLUST));

	/*get file path*/
	memset(data->full_path, 0, FULL_PATH_LEN);
	strcpy(data->full_path, plist->topdir);

	if (data->full_path[strlen(data->full_path) - 1] != ':')
		data->full_path[strlen(data->full_path)] = '/';

	get_pos_seq_file_path(path_buff, data->full_path);
	data->cursor.path = data->full_path;
	iter->cursor = &data->cursor;

	SYS_LOG_DBG("full_path:%s\n", data->full_path);

exit:
	if(path_buff)
		mem_free(path_buff);
	if(zdp)
		mem_free(zdp);
	if(entry)
		mem_free(entry);
	return res;
}
#else
/*url:bycluster:/SD://cluster:2/320/12992420/.mp3*/
static void _file_format_get(char *name, char **file_format)
{
	int len = strlen(name);
	while (len) {
		if (name[--len] == '.') {
			*file_format = name + len;
			break;
		}
	}
}
static int file_dirname_get(struct play_list_t *plist, struct iterator *iter)
{
	struct file_iterator_data *data = iter->data;
	int res = -ENOENT;
	uint16_t times = 0;
	DIR* dp = NULL;
	char *file_format = NULL;
	struct fs_dir_t *zdp = mem_malloc(sizeof(struct fs_dir_t));
	struct fs_dirent *entry = mem_malloc(sizeof(struct fs_dirent));

	if (!zdp || !entry || !plist->dir_file_seq_num)
		goto exit;

	/*read file name*/
	res = fs_opendir_cluster(zdp, plist->topdir, plist->folder_info[plist->folder_seq_num]->cur_cluster, 0);
	dp = zdp->dirp;
	if (!res) {
		do {
			memset(entry, 0, sizeof(struct fs_dirent));
			res = fs_readdir(zdp, entry);
			if (res || entry->name[0] == 0) {
				SYS_LOG_ERR("fs_readdir failed (res=%d), skip this\n", res);
				fs_closedir(zdp);
				goto exit;
			}
			/* filter out unmatch directory or file */
			if (plist->match_fn && entry->type == FS_DIR_ENTRY_FILE && plist->match_fn(entry->name, 0))
				times++;
		} while (times < plist->dir_file_seq_num);
		SYS_LOG_INF("file name:%s\n", entry->name);
		fs_closedir(zdp);
	} else {
		SYS_LOG_ERR("fs_opendir failed (res=%d)\n", res);
		fs_closedir(zdp);
		goto exit;
	}
	/* get file format */
	_file_format_get(entry->name, &file_format);
	/*get file path*/
	memset(data->full_path, 0, FULL_PATH_LEN);
	snprintf(data->full_path, FULL_PATH_LEN, "%s%s/%s%u/%lu/%u/%s", OPEN_MODE, plist->topdir, CLUSTER,
		plist->folder_info[plist->folder_seq_num]->cur_cluster, dp->blk_ofs, entry->size, entry->name);

	data->cursor.path = data->full_path;
	iter->cursor = &data->cursor;

	SYS_LOG_DBG("full_path:%s\n", data->full_path);

exit:
	if(zdp)
		mem_free(zdp);
	if(entry)
		mem_free(entry);
	return res;

}
#endif
static const void *file_iterator_set_track_no(struct iterator *iter, uint16_t track_no)
{
	struct file_iterator_data *data = iter->data;
	struct play_list_t *plist = get_play_list();

	if (!plist || !plist->sum_file_count)
		return NULL;

	if (track_no == 0 || track_no > plist->sum_file_count)
		return NULL;

	calc_track_no_playlist_info(plist, track_no);

	if (file_dirname_get(plist, iter))
		return NULL;

	data->has_query_next = 0;
	data->has_query_prev = 0;
	return data->full_path;
}

static int file_iterator_set_mode(struct iterator *iter, uint8_t mode)
{
	struct play_list_t *plist = get_play_list();

	if (!plist)
		return -ENXIO;

	plist->mode = mode;

	return 0;
}
static const void *file_iterator_prev_folder(struct iterator *iter, uint16_t *track_no)
{
	struct file_iterator_data *data = iter->data;
	struct play_list_t *plist = get_play_list();

	if (!plist || !plist->sum_file_count)
		return NULL;

	calc_next_folder_playlist_info(plist, 0);

	if (file_dirname_get(plist, iter))
		return NULL;
	*(uint16_t *)track_no = plist->file_seq_num;
	return data->full_path;
}

static const void *file_iterator_next_folder(struct iterator *iter, uint16_t *track_no)
{
	struct file_iterator_data *data = iter->data;
	struct play_list_t *plist = get_play_list();

	if (!plist || !plist->sum_file_count)
		return NULL;

	calc_next_folder_playlist_info(plist, 1);

	if (file_dirname_get(plist, iter))
		return NULL;
	*(uint16_t *)track_no = plist->file_seq_num;
	return data->full_path;
}

static const void *next(struct iterator *iter, bool force_switch, uint16_t *track_no)
{
	struct file_iterator_data *data = iter->data;
	struct play_list_t *plist = get_play_list();

	if (!plist || !plist->sum_file_count)
		return NULL;
	/*no cycle in mode 2*/
	if (plist->mode == 2 && plist->file_seq_num == plist->sum_file_count && !force_switch)
		return NULL;
	/*folder cycle in mode 3*/
	if (plist->mode == 3 &&
		plist->dir_file_seq_num >= plist->folder_info[plist->folder_seq_num]->dir_file_count)
		plist->file_seq_num -= plist->dir_file_seq_num;

	calc_next_playlist_info(plist, 1);

	if (file_dirname_get(plist, iter))
		return NULL;
	if (track_no)
		*(uint16_t *)track_no = plist->file_seq_num;
	return data->full_path;
}

static int file_iterator_has_next(struct iterator *iter)
{
	struct file_iterator_data *data = iter->data;

	if (data->has_query_next)
		return iter->cursor != NULL;

	data->has_query_next = 1;
	return next(iter, false, NULL) != NULL;
}

static const void *file_iterator_next(struct iterator *iter, bool force_switch, uint16_t *track_no)
{
	struct file_iterator_data *data = iter->data;
	struct play_list_t *plist = get_play_list();

	if (!plist || !plist->sum_file_count)
		return NULL;

	if (data->has_query_next) {
		const struct file_iterator_cursor *cursor = iter->cursor;
		if (cursor) {
			data->has_query_next = 0;
			if (track_no)
				*(uint16_t *)track_no = plist->file_seq_num;
			return cursor->path;
		}

		return NULL;
	}
	return next(iter, force_switch, track_no);
}

static const void *prev(struct iterator *iter, uint16_t *track_no)
{
	struct file_iterator_data *data = iter->data;
	struct play_list_t *plist = get_play_list();

	if (!plist || !plist->sum_file_count)
		return NULL;
	/*folder cycle in mode 3*/
	if (plist->mode == 3 && plist->dir_file_seq_num == 1)
		plist->file_seq_num += plist->folder_info[plist->folder_seq_num]->dir_file_count;

	calc_next_playlist_info(plist, 0);

	if (file_dirname_get(plist, iter))
		return NULL;
	if (track_no)
		*(uint16_t *)track_no = plist->file_seq_num;
	return data->full_path;

}

static int file_iterator_has_prev(struct iterator *iter)
{
	struct file_iterator_data *data = iter->data;

	if (data->has_query_prev)
		return iter->cursor != NULL;

	data->has_query_prev = 1;
	return prev(iter, NULL) != NULL;
}

static const void *file_iterator_prev(struct iterator *iter, uint16_t *track_no)
{
	struct file_iterator_data *data = iter->data;
	struct play_list_t *plist = get_play_list();

	if (!plist || !plist->sum_file_count)
		return NULL;

	if (data->has_query_prev) {
		const struct file_iterator_cursor *cursor = iter->cursor;
		if (cursor) {
			data->has_query_prev = 0;
			if (track_no)
				*(uint16_t *)track_no = plist->file_seq_num;
			return cursor->path;
		}

		return NULL;
	}
	return prev(iter, track_no);
}

static int _back_to_topdir(struct file_iterator_data *data)
{
	int res = 0;
	int i;

	/* close all sub-directories, and reset topdir state */
	for (i = data->level; i >= 0; i--)
		fs_closedir(data->dirs[i]);

	data->level = -1;
	data->fname_len = 0;
	data->full_len = data->dname_len[0];
	data->full_path[data->full_len] = 0;

	if (data->full_path[data->full_len - 1] == '/')
		data->full_path[data->full_len - 1] = 0;

	res = fs_opendir(data->dirs[0], data->full_path);
	if (res) {
		SYS_LOG_ERR("fs_opendir %s failed (res=%d)\n", data->full_path, res);
		return res;
	}
	if (data->full_path[data->full_len - 1] != ':')
		data->full_path[data->full_len - 1] = '/';

	data->level = 0;
	return 0;
}

static int file_iterator_playlist_init(struct play_list_t *plist, struct file_iterator_data *data, const void *param)
{
	int res = -ENOENT;
	const struct file_iterator_param *iter_param = (struct file_iterator_param *)param;
	struct fs_dir_t *zdp = NULL;
	DIR* dp = NULL;

	zdp = mem_malloc(sizeof(struct fs_dir_t));
	if (!zdp)
		return res;

	if (data->full_path[data->full_len - 1] == '/')
		data->full_path[data->full_len - 1] = 0;
	res = fs_opendir(zdp, data->full_path);
	if (res) {
		SYS_LOG_ERR("fs_opendir %s failed (res=%d)\n", data->full_path, res);
		goto exit;
	}
	if (data->full_path[data->full_len - 1] != ':')
		data->full_path[data->full_len - 1] = '/';

	dp = zdp->dirp;

	plist->topdir = iter_param->topdir;
	plist->match_fn = iter_param->match_fn;
	plist->csize = dp->obj.fs->csize;
#if CONFIG_SUPPORT_FILE_FULL_NAME
	plist->folder_info[0]->far_cluster = 0;
	plist->folder_info[0]->blk_ofs = 0;
	if (dp->obj.fs->fs_type <= FS_FAT16) {
		plist->fs_type = 1;
		plist->folder_info[0]->far_cluster = FAT16_FAR_CLUST;
	}
#endif
	plist->folder_info[0]->cur_cluster = dp->clust;

	plist->folder_info[0]->dir_file_count = 0;
	plist->folder_info[0]->dir_layer = 0;
	SYS_LOG_INF("fs info %s cur_cluster=%d,csize=%d\n",
		data->full_path, plist->folder_info[0]->cur_cluster, plist->csize);
	fs_closedir(zdp);
exit:
	if (zdp)
		mem_free(zdp);
	return res;
}

#if CONFIG_SUPPORT_FILE_FULL_NAME
static int updata_far_cluster_blkofs(struct play_list_t *plist, struct fs_dir_t *zdp, uint8_t folder_index, const char *path)
{
	DIR* dp = zdp->dirp;
	int res = 0;
	struct fs_dir_t *tempdp = NULL;
	char *path_buff = NULL;
	char *buf = NULL;

	plist->folder_info[folder_index]->far_cluster = dp->clust;
	plist->folder_info[folder_index]->blk_ofs = dp->blk_ofs;

	if (dp->blk_ofs >= plist->csize * 512) {/*Sector size 512*/
		tempdp = mem_malloc(sizeof(struct fs_dir_t));
		path_buff = mem_malloc(strlen(path) + 1);
		if (!tempdp || !path_buff)
			goto exit;

		strcpy(path_buff, path);
		buf = strrchr(path_buff, '/');
		if (buf) {
			buf[0] = 0;
		} else {
			buf = strrchr(path_buff, ':');
			buf[1] = 0;
		}

		res = fs_opendir(tempdp, path_buff);
		if (res) {
			SYS_LOG_WRN("fs_opendir %s failed (res=%d)\n", path_buff, res);
		}
		dp = tempdp->dp;
		plist->folder_info[folder_index]->far_cluster = dp->clust;
		fs_closedir(tempdp);
	}
	SYS_LOG_DBG("folder_index=%d,far_cluster=%d,blk_ofs=%d\n",
		folder_index, plist->folder_info[folder_index]->far_cluster , plist->folder_info[folder_index]->blk_ofs);
exit:
	if (tempdp)
		mem_free(tempdp);
	if (path_buff)
		mem_free(path_buff);
	return res;
}
#endif

static int file_iterator_playlist_set(struct play_list_t *plist, struct fs_dir_t *zdp, uint8_t folder_index, uint8_t layer)
{
	DIR* dp = zdp->dirp;

	plist->folder_info[folder_index]->cur_cluster = dp->clust;
	plist->folder_info[folder_index]->dir_file_count = 0;
	plist->folder_info[folder_index]->dir_layer = layer;
	SYS_LOG_DBG("folder_index=%d,cur_cluster= %lu,layer=%d\n", folder_index, dp->clust, layer);
	return 0;
}
/*url:bycluster:SD:/cluster:2/320/12992420*/
static int get_cursor_info(const char *path, uint32_t *cluster, uint32_t *blk_ofs, uint32_t *file_size)
{
	char *str = NULL;
	char *clust = NULL;
	char *blk = NULL;
	char *temp_url = NULL;
	int res = -EINVAL;

	temp_url = mem_malloc(strlen(path) + 1);
	if (!temp_url)
		goto exit;

	strcpy(temp_url, path);
	str = strstr(temp_url,"/cluster:");
	if (!str)
		goto exit;

	str += strlen("/cluster:");
	clust = str;
	str = strchr(str, '/');
	if (!str)
		goto exit;
	str[0] = 0;

	str++;
	blk = str;
	str = strchr(str, '/');
	if (!str)
		goto exit;
	str[0] = 0;
	str++;

	*cluster = atoi(clust);
	*blk_ofs = atoi(blk);
	*file_size = atoi(str);

	SYS_LOG_DBG("cluster=%u,blk_ofs=%u,file_size=%u\n", *cluster, *blk_ofs, *file_size);
	res = 0;

exit:
	if (temp_url)
		mem_free(temp_url);
	return res;

}

static const char *file_iterator_scan_disk(struct play_list_t *plist, struct iterator *iter, const void *param)
{
	struct file_iterator_data *data = iter->data;
	const struct file_iterator_param *iter_param = (struct file_iterator_param *)param;
	const struct file_iterator_cursor *cursor = NULL;
	int res = -ENOENT;
	uint16_t len;
	uint8_t is_file_type = 1;
	uint8_t has_sub_folder = 0;
	uint32_t cursor_cluster = 0;
	uint32_t cursor_blk_ofs = 0;
	uint32_t cursor_file_size = 0;
	uint32_t temp_cluster = 0;
	DIR* dp = NULL;

	if (!iter_param) {
		return NULL;
	}

	cursor = iter_param->cursor;
	if (!cursor) {
		return NULL;
	}

	iter->cursor = NULL;
	if (data->level < 0)
		return NULL;

	if (cursor->path)
		get_cursor_info(cursor->path, &cursor_cluster, &cursor_blk_ofs, &cursor_file_size);

	dp = data->dirs[data->level]->dirp;
	temp_cluster = dp->clust;

	do {
		res = fs_readdir(data->dirs[data->level], data->dirent);
		if (res)
			SYS_LOG_ERR("fs_readdir failed (res=%d), skip this\n", res);

		/*
		 * if fs_readdir failed or end-of-dir, skip this directory and
		 * go up one level.
		 *
		 * name[0] == 0 means end-of-dir
		 */

		if (res || data->dirent->name[0] == 0) {
			fs_closedir(data->dirs[data->level]);

			if (data->level == 0 && (is_file_type == 0 || has_sub_folder == 0)) {
				data->level = -1;
				return NULL;
			}

			if (is_file_type && has_sub_folder) {
				is_file_type = 0;
				has_sub_folder = 0;
				/*remove file name*/
				data->full_len -= data->fname_len;
				data->full_path[data->full_len] = 0;
				data->fname_len = 0;
				if (data->full_path[data->full_len - 1] == '/')
					data->full_path[data->full_len - 1] = 0;
				SYS_LOG_DBG("repeat_opendir %s \n", data->full_path);
				res = fs_opendir(data->dirs[data->level], data->full_path);
				if (res) {
					SYS_LOG_WRN("repeat_opendir %s failed (res=%d)\n", data->full_path, res);
				}
				if (data->full_path[data->full_len - 1] == 0)
					data->full_path[data->full_len - 1] = '/';
				continue;
			}
			/* up one level to read dir*/
			data->full_len -= data->fname_len + data->dname_len[data->level];
			data->full_path[data->full_len] = 0;
			data->fname_len = 0;
			data->level--;
			SYS_LOG_DBG("up to dir %s\n", data->full_path);
			is_file_type = 0;
			continue;
		}

		/* FIXME: skip hidden diretory or file ? */
		if (data->dirent->name[0] == '.')
			continue;

		/* manage the full path */
		len = (uint16_t)strlen(data->dirent->name);
		data->full_len -= data->fname_len;
		if (data->full_len + len >= FULL_PATH_LEN - 1) {
			SYS_LOG_WRN("too long path, exceed %u bytes (%s)\n",
					data->full_len + len - sizeof(data->full_path), data->dirent->name);
			data->full_len += data->fname_len;
			continue;
		}

		memcpy(&data->full_path[data->full_len], data->dirent->name, len);
		data->fname_len = len;
		data->full_len += len;
		data->full_path[data->full_len] = 0;

		/* filter out unmatch directory or file */
		if (data->match_fn && !data->match_fn(data->full_path,
					data->dirent->type == FS_DIR_ENTRY_DIR)) {
			SYS_LOG_DBG("filter out %s\n", data->full_path);
			continue;
		}

		/* This is a file, file count ++ in file type mode */
		if (data->dirent->type == FS_DIR_ENTRY_FILE && is_file_type) {
			plist->folder_info[plist->sum_folder_count]->dir_file_count++;
			plist->sum_file_count++;

			/*set cursor*/
			if (cursor && cursor->path && 
				((temp_cluster == cursor_cluster && dp->blk_ofs == cursor_blk_ofs && (uint32_t)(data->dirent->size) == cursor_file_size)
				|| !strcmp(cursor->path, data->full_path))) {
				plist->dir_file_seq_num = plist->folder_info[plist->sum_folder_count]->dir_file_count;
				plist->file_seq_num = plist->sum_file_count;
				plist->folder_seq_num = plist->sum_folder_count;
				SYS_LOG_INF("cur file_seq_num=%d,dir_file_seq_num=%d,folder_seq_num=%d\n",
					plist->file_seq_num, plist->dir_file_seq_num, plist->folder_seq_num);
			}

			if (plist->sum_file_count == MAX_SUPPORT_FILE_CNT) {
				SYS_LOG_WRN("exceed max count\n");
				break;
			}
			continue;
		}

		/* This is a file, file count no change in dir type mode */
		if (data->dirent->type == FS_DIR_ENTRY_FILE && is_file_type == 0) {
			continue;
		}

		/* This is a dir, set had sub folder flag in file type mode */
		if (data->dirent->type == FS_DIR_ENTRY_DIR && is_file_type) {
			has_sub_folder = 1;
			continue;
		}

		if (data->level + 1 >= data->max_level) {
			SYS_LOG_DBG("exceed max level (%d)\n", data->max_level);
			continue;
		}

		if (plist->sum_folder_count + 1 >= CONFIG_PLIST_SUPPORT_FOLDER_CNT) {
			SYS_LOG_INF("exceed max folder(%d)\n", CONFIG_PLIST_SUPPORT_FOLDER_CNT);
			break;
		}
		/* This is directory, down one level if everthing ok */
		res = fs_opendir(data->dirs[data->level + 1], data->full_path);
		if (res) {
			SYS_LOG_WRN("fs_opendir %s failed (res=%d)\n", data->full_path, res);
			continue;
		}

		/*set playlist folder structor*/
		plist->sum_folder_count++;
	#if CONFIG_SUPPORT_FILE_FULL_NAME
		updata_far_cluster_blkofs(plist, data->dirs[data->level], plist->sum_folder_count, data->full_path);
	#endif

		file_iterator_playlist_set(plist, data->dirs[data->level + 1], plist->sum_folder_count, data->level + 1);
		/*read file first in a new dir*/
		is_file_type = 1;
		/* append a seperator */
		data->full_path[data->full_len] = '/';
		data->full_path[++data->full_len] = 0;
		data->dname_len[++data->level] = data->fname_len + 1;
		data->fname_len = 0;

		/*open dir need to get cluster*/
		dp = data->dirs[data->level]->dirp;
		temp_cluster = dp->clust;

		SYS_LOG_DBG("down to dir %s\n", data->full_path);
	} while (1);

	/* clear prev query flag */
	data->has_query_prev = 0;

	return data->full_path;
}

static int file_iterator_update_playlist(struct play_list_t *plist, struct iterator *iter, const void *param)
{
	struct file_iterator_data *data = iter->data;

	int res = -ENOENT;

	file_iterator_playlist_init(plist, data, param);

	res = _back_to_topdir(data);
	if (res)
		return res;
	/* scan disk to update playlist */
#if CONFIG_SYS_LOG_DEFAULT_LEVEL >= 3
	uint32_t begin = k_cycle_get_32();
#endif
	file_iterator_scan_disk(plist, iter, param);
#if CONFIG_SYS_LOG_DEFAULT_LEVEL >= 3
	SYS_LOG_INF("scan disk case %u us \n", (k_cycle_get_32() - begin)/24);
#endif

	return res;
}

static int check_cursor_exist(struct play_list_t *plist, const void *iter_cursor)
{
	int i = 0;

	for (i = 0; i <= plist->sum_folder_count; i++) {
		if (plist->folder_info[i]->dir_file_count)
			break;
	}

	if (i <= plist->sum_folder_count) {
		plist->file_seq_num = 1;
		plist->folder_seq_num = i;
		plist->dir_file_seq_num = 1;/*start from 1st*/
		SYS_LOG_INF("cursor no exist,start from folder(%d)\n", i);
	}

	return 0;
}

static int file_iterator_set_cursor(struct iterator *iter, const void *iter_cursor)
{
	struct file_iterator_data *data = iter->data;
	const struct file_iterator_cursor *cursor = iter_cursor;
	int res = 0;
	struct play_list_t *plist = get_play_list();

	if (!plist || !plist->sum_file_count)
		return -ENOENT;

	if (cursor->path == NULL) {
		check_cursor_exist(plist, cursor);
		res = file_dirname_get(plist, iter);
		if (!res)
			data->has_query_next = 1;
	}
	return res;
}

static int file_iterator_init(struct iterator *iter, const void *param)
{
	const struct file_iterator_param *iter_param = (struct file_iterator_param *)param;
	const struct file_iterator_cursor *cursor = NULL;
	struct file_iterator_data *data = NULL;
	uint8_t stat = STA_NODISK;
	int res = -ENOMEM;
	int i = 0;

	if (!iter_param) {
		return -EINVAL;
	}

	if (!iter_param->topdir || iter_param->max_level <= 0)
		return -EINVAL;

	cursor = iter_param->cursor;

	fs_disk_detect(iter_param->topdir, &stat);
	if (stat != STA_DISK_OK) {
		SYS_LOG_ERR("disk not found (%s)\n", iter_param->topdir);
		return -ENODEV;
	}

	data = mem_malloc(sizeof(*data));
	if (!data)
		return -ENOMEM;

	memset(data, 0, sizeof(*data));
	data->match_fn = iter_param->match_fn;
	data->max_level = (uint16_t)iter_param->max_level;
	data->level = -1;

	data->dirent = mem_malloc(sizeof(*data->dirent));
	if (!data->dirent)
		goto err_out;

	for (i = 0; i < data->max_level; i++) {
		data->dirs[i] =  mem_malloc(sizeof(struct fs_dir_t));
		if (!data->dirs[i])
			goto err_out;
	}

	data->dname_len = mem_malloc(sizeof(*data->dname_len) * data->max_level);
	if (!data->dname_len)
		goto err_out;

	data->full_path = mem_malloc(FULL_PATH_LEN);
	if (!data->full_path)
		goto err_out;

	if (!play_list) {
		play_list = mem_malloc(sizeof(*play_list));
		if (!play_list)
			goto err_out;

		for (i = 0; i < CONFIG_PLIST_SUPPORT_FOLDER_CNT; i++) {
			play_list->folder_info[i] =  mem_malloc(sizeof(struct folder_info_t));
			if (!play_list->folder_info[i])
				goto err_out;
		}
	}

	strcpy(data->full_path, iter_param->topdir);
	data->dname_len[0] = strlen(iter_param->topdir);
	data->full_len = data->dname_len[0];
	if (data->full_path[data->full_len - 1] != ':' &&
		data->full_path[data->full_len - 1] != '/') {
		data->full_path[data->full_len++] = '/';
		data->dname_len[0]++;
	}

	data->cursor.path = data->full_path;

	/* set_cursor may access data */
	iter->data = data;

	res = file_iterator_update_playlist(play_list, iter, iter_param);
	if (res)
		goto err_out;

	SYS_LOG_INF("sum_file_count=%d,sum_folder_count=%d\n", play_list->sum_file_count, play_list->sum_folder_count + 1);

	if (cursor)
		iter->ops->set_cursor(iter, cursor);
#if 0
	for (i = 0; i <= play_list->sum_folder_count; i++) {
		print_buffer(play_list->folder_info[i], 1, sizeof(struct folder_info_t), sizeof(struct folder_info_t), -1);
	}
#endif

	for (i = data->level; i >= 0; i--)
		fs_closedir(data->dirs[i]);

	if (data->dirent) {
		mem_free(data->dirent);
		data->dirent = NULL;
	}

	for (i = 0; i < data->max_level; i++) {
		if (data->dirs[i]) {
			mem_free(data->dirs[i]);
			data->dirs[i] = NULL;
		}
	}

	if (data->dname_len) {
		mem_free(data->dname_len);
		data->dname_len = NULL;
	}

	return 0;

err_out:
	for (i = data->level; i >= 0; i--)
		fs_closedir(data->dirs[i]);

	if (data->dirent) {
		mem_free(data->dirent);
		data->dirent = NULL;
	}

	for (i = 0; i < data->max_level; i++) {
		if (data->dirs[i]) {
			mem_free(data->dirs[i]);
			data->dirs[i] = NULL;
		}
	}

	if (data->dname_len) {
		mem_free(data->dname_len);
		data->dname_len = NULL;
	}

	if (data->full_path) {
		mem_free(data->full_path);
		data->full_path = NULL;
	}
	mem_free(data);

	if (play_list) {
		for (i = 0; i < CONFIG_PLIST_SUPPORT_FOLDER_CNT; i++) {
			if (play_list->folder_info[i]) {
				mem_free(play_list->folder_info[i]);
				play_list->folder_info[i] = NULL;
			}
		}

		mem_free(play_list);
		play_list = NULL;
	}

	return res;
}

static const struct iterator_ops file_iterator_ops = {
	.init = file_iterator_init,
	.destroy = file_iterator_destroy,
	.has_next = file_iterator_has_next,
	.next = file_iterator_next,
	.has_prev = file_iterator_has_prev,
	.prev = file_iterator_prev,
	.set_cursor = file_iterator_set_cursor,
	.next_folder = file_iterator_next_folder,
	.prev_folder = file_iterator_prev_folder,
	.set_track_no = file_iterator_set_track_no,
	.set_mode = file_iterator_set_mode,
	.get_plist_info = file_iterator_get_plist_info,
};

struct iterator *file_iterator_create(file_iterator_param_t *param)
{
	return iterator_create((struct iterator_ops *)&file_iterator_ops,param);
}
