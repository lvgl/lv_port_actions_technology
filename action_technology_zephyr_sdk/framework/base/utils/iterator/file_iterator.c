#include <zephyr.h>
#include <fs/fs.h>
#include <disk_access.h>
#include <mem_manager.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <iterator/file_iterator.h>

#define SAVE_CURSOR_SERACHING_LIST

#define HISTORY_FNAME "_iter.log"
#define MIN_ITEM_SIZE 6
#define MAX_DIR_LEVEL 9
#define MAX_ITEM_SIZE (MAX_URL_LEN + 6)
#define FULL_PATH_LEN (MAX_URL_LEN + 2)

/*
 * item structure in history file list.
 *
 *---------------------------------------------
 * item len | path + '\0' | item len | '\n'   |
 *---------------------------------------------
 * 2 byte   |             | 2 byte   | 1 byte |
 *---------------------------------------------
 */

struct file_iterator_data {
	uint8_t has_query_next : 1; /* indicate whether has_next has been called */
	uint8_t has_query_prev : 1; /* indicate whether has_prev has been called */

	/* cached dir structures */
	int16_t level;
	uint16_t max_level;
	fs_dir_t *dirs[MAX_DIR_LEVEL];

	/* used to compose new full path */
	uint16_t *dname_len; /* used to store name len of every level directories */
	uint16_t fname_len;  /* name len of current file */
	uint16_t full_len;   /* name len of full path */
	char *full_path;  /* store the full path */

	struct file_iterator_cursor cursor;

	int (*match_fn)(const char *path, int is_dir);

	/* travel item list */
	fs_file_t *list_file;
	uint16_t num_items;
	uint16_t item_idx;
	uint16_t item_len;

	union {
		struct fs_dirent *dirent;
		char *s_item;
	};
};

static int open_travel_filist(struct file_iterator_data *data)
{
	char path[24];
	const char *s = strchr(data->full_path, ':');
	if (!s)
		return -EINVAL;

	data->list_file = mem_malloc(sizeof(*data->list_file));
	if (!data->list_file)
		return -ENOMEM;

	int len = s - data->full_path + 1;
	memcpy(path, data->full_path, len);
	strcpy(&path[len], HISTORY_FNAME);

	int res = fs_open(data->list_file, path);
	if (res) {
		SYS_LOG_ERR("%s fs_open failed (res=%d)\n", path, res);
		mem_free(data->list_file);
		data->list_file = NULL;
		return res;
	}

	fs_trunc_quick(data->list_file, 0);
	data->num_items = 0;
	return 0;
}

static void close_travel_filist(struct file_iterator_data *data)
{
	if (data->list_file) {
		fs_close(data->list_file);
		mem_free(data->list_file);
		data->list_file = NULL;
	}
}

static int save_next_to_filist(struct file_iterator_data *data)
{
	uint16_t len = data->full_len + 6;
	int res;

	if (!data->list_file)
		return -ENOENT;

	memcpy(data->s_item, &len, sizeof(len));
	memcpy(data->s_item + 2, data->full_path, data->full_len + 1);
	memcpy(data->s_item + 3 + data->full_len, &len, sizeof(len));
	data->s_item[len - 1] = '\n';

	res = fs_write(data->list_file, data->s_item, len);
	if (res != len) {
		SYS_LOG_ERR("failed to save %s (res=%d)\n", data->full_path, res);
		return res;
	}

	data->item_len = len;
	data->item_idx = data->num_items++;

	SYS_LOG_DBG("%s\n", data->full_path);
	return res;
}

static const char *get_next_from_filist(struct file_iterator_data *data)
{
	uint16_t len;
	int res;

	if (!data->list_file)
		return NULL;

	if (data->num_items <= 0 || data->item_idx >= data->num_items - 1)
		return NULL;

	res = fs_read(data->list_file, data->s_item, 2);
	if (res != 2) {
		SYS_LOG_ERR("fs_read failed (res=%d)\n", res);
		return NULL;
	}

	memcpy(&len, data->s_item, sizeof(len));
	if (len < MIN_ITEM_SIZE || len > MAX_ITEM_SIZE) {
		SYS_LOG_ERR("invalid item len %d\n", len);
		return NULL;
	}

	res = fs_read(data->list_file, data->s_item + 2, len - 2);
	if (res != len - 2) {
		SYS_LOG_ERR("fs_read 2 failed (res=%d)\n", res);
		return NULL;
	}

	data->item_len = len;
	data->item_idx++;
	return data->s_item + 2;
}

static const char *get_prev_from_filist(struct file_iterator_data *data)
{
	uint16_t len;
	int res;

	if (!data->list_file)
		return NULL;

	if (data->num_items <= 0 || data->item_idx == 0)
		return NULL;

	res = fs_seek(data->list_file, -(data->item_len + 3), FS_SEEK_CUR);
	if (res) {
		SYS_LOG_ERR("fs_seek 1 failed (res=%d)\n", res);
		return NULL;
	}

	res = fs_read(data->list_file, data->s_item, 3);
	if (res != 3) {
		SYS_LOG_ERR("fs_read 1 failed (res=%d)\n", res);
		return NULL;
	}

	memcpy(&len, data->s_item, sizeof(len));
	if (len < MIN_ITEM_SIZE || len > MAX_ITEM_SIZE) {
		SYS_LOG_ERR("invalid item len %d\n", len);
		return NULL;
	}

	res = fs_seek(data->list_file, -len, FS_SEEK_CUR);
	if (res) {
		SYS_LOG_ERR("fs_seek 2 failed (res=%d)\n", res);
		return NULL;
	}

	res = fs_read(data->list_file, data->s_item, len);
	if (res != len) {
		SYS_LOG_ERR("fs_read 2 failed (res=%d)\n", res);
		return NULL;
	}

	data->item_len = len;
	data->item_idx--;
	return data->s_item + 2;
}

static int file_iterator_init(struct iterator *iter, const void *param)
{
	const struct file_iterator_param *iter_param = (struct file_iterator_param *)param;
	const struct file_iterator_cursor *cursor = iter_param->cursor;
	struct file_iterator_data *data = NULL;
	uint8_t stat = STA_NODISK;
	int res = -ENOMEM;
	int i = 0;

	if (!iter_param || !iter_param->topdir || iter_param->max_level <= 0)
		return -EINVAL;

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

	data->s_item = mem_malloc(max(MAX_ITEM_SIZE, sizeof(*data->dirent)));
	if (!data->s_item)
		goto err_out;

	for (i = 0; i < data->max_level; i++) {
		data->dirs[i] =  mem_malloc(sizeof(fs_dir_t));
		if (!data->dirs[i])
			goto err_out;
	}

	data->dname_len = mem_malloc(sizeof(*data->dname_len) * data->max_level);
	if (!data->dname_len)
		goto err_out;

	data->full_path = mem_malloc(FULL_PATH_LEN);
	if (!data->full_path)
		goto err_out;

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

	open_travel_filist(data);

	if (cursor && cursor->path)
		iter->ops->set_cursor(iter, cursor);

	if (data->level < 0) {
		res = fs_opendir(data->dirs[0], iter_param->topdir);
		if (res) {
			SYS_LOG_ERR("topdir %s open failed\n", iter_param->topdir);
			goto err_out;
		}

		data->level = 0;
	}

	return 0;
err_out:
	close_travel_filist(data);

	if (data->s_item)
		mem_free(data->s_item);

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
	return res;
}

static int file_iterator_destroy(struct iterator *iter)
{
	struct file_iterator_data *data = iter->data;
	int i;

	for (i = data->level; i >= 0; i--)
		fs_closedir(data->dirs[i]);

	close_travel_filist(data);
	mem_free(data->s_item);

	for (i = 0; i < data->max_level; i++) {
		if (data->dirs[i]) {
			mem_free(data->dirs[i]);
		}
	}
	mem_free(data->dname_len);
	mem_free(data->full_path);
	mem_free(data);
	return 0;
}

static const void *next(struct iterator *iter)
{
	struct file_iterator_data *data = iter->data;
	uint16_t len;
	int res = -ENOENT;

	/* search the history first */
	data->cursor.path = get_next_from_filist(data);
	if (data->cursor.path) {
		iter->cursor = &data->cursor;
		return data->cursor.path;
	}

	iter->cursor = NULL;
	if (data->level < 0)
		return NULL;

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

			if (data->level == 0) {
				data->level = -1;
				return NULL;
			}

			/* up one level */
			data->full_len -= data->fname_len + data->dname_len[data->level];
			data->full_path[data->full_len] = 0;
			data->fname_len = 0;
			data->level--;
			SYS_LOG_DBG("up to dir %s\n", data->full_path);
			continue;
		}

		/* FIXME: skip hidden diretory or file ? */
		if (data->dirent->name[0] == '.')
			continue;

		/* manage the full path */
		len = (uint16_t)strlen(data->dirent->name);
		data->full_len -= data->fname_len;
		if (data->full_len + len >= FULL_PATH_LEN - 1) {
			SYS_LOG_WRN("too long path, exceed %zu bytes (%s)\n",
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

		/* This is file, return the full path */
		if (data->dirent->type == FS_DIR_ENTRY_FILE) {
			data->cursor.path = data->full_path;
			iter->cursor = &data->cursor;
			break;
		}

		/* This is directory, down one level if everthing ok */
		if (data->level + 1 >= data->max_level) {
			SYS_LOG_DBG("exceed max level (%u)\n", data->max_level);
			continue;
		}

		res = fs_opendir(data->dirs[data->level + 1], data->full_path);
		if (res) {
			SYS_LOG_WRN("fs_opendir %s failed (res=%d)\n", data->full_path, res);
			continue;
		}

		/* append a seperator */
		data->full_path[data->full_len] = '/';
		data->full_path[++data->full_len] = 0;
		data->dname_len[++data->level] = data->fname_len + 1;
		data->fname_len = 0;

		SYS_LOG_DBG("down to dir %s\n", data->full_path);
	} while (1);

	/* save history */
	save_next_to_filist(data);

	/* clear prev query flag */
	data->has_query_prev = 0;

	return data->full_path;
}

static int file_iterator_has_next(struct iterator *iter)
{
	struct file_iterator_data *data = iter->data;

	if (data->has_query_next)
		return iter->cursor != NULL;

	data->has_query_next = 1;
	return next(iter) != NULL;
}

static const void *file_iterator_next(struct iterator *iter)
{
	struct file_iterator_data *data = iter->data;

	if (data->has_query_next) {
		const struct file_iterator_cursor *cursor = iter->cursor;
		if (cursor) {
			data->has_query_next = 0;
			return cursor->path;
		}

		return NULL;
	}

	return next(iter);
}

static const void *prev(struct iterator *iter)
{
	struct file_iterator_data *data = iter->data;

	data->cursor.path = get_prev_from_filist(data);
	if (data->cursor.path) {
		/* clear next query flag */
		data->has_query_next = 0;

		iter->cursor = &data->cursor;
	} else {
		iter->cursor = NULL;
	}

	return data->cursor.path;
}

static int file_iterator_has_prev(struct iterator *iter)
{
	struct file_iterator_data *data = iter->data;

	if (data->has_query_prev)
		return iter->cursor != NULL;

	data->has_query_prev = 1;
	return prev(iter) != NULL;
}

static const void *file_iterator_prev(struct iterator *iter)
{
	struct file_iterator_data *data = iter->data;

	if (data->has_query_prev) {
		const struct file_iterator_cursor *cursor = iter->cursor;
		if (cursor) {
			data->has_query_prev = 0;
			return cursor->path;
		}

		return NULL;
	}

	return prev(iter);
}

static int _validate_cursor(struct file_iterator_data *data, const struct file_iterator_cursor *cursor)
{
	int res = 0;

	/* cursor must be below topdir */
	if (strncmp(data->full_path, cursor->path, data->dname_len[0])) {
		SYS_LOG_ERR("cursor %s should below topdir %s, %d\n",
				cursor->path, data->full_path, data->dname_len[0]);
		return -EINVAL;
	}

	/* cursor must be a file */
	res = fs_stat(cursor->path, data->dirent);
	if (res || data->dirent->type != FS_DIR_ENTRY_FILE) {
		SYS_LOG_ERR("cursor %s not found (res=%d)\n", cursor->path, res);
		return -ENOENT;
	}

	/* FIXME: cursor should not be filtered out: test the file and the directory.
	 *
	 * If a directory is filtered out in match_fn, then the sub-directories and bottom
	 * files should also be filtered out in match_fn. But some implementions of match_fn
	 * do not obey this, and they match the files and directories separately, so just
	 * test the path both as file and directory to validate the cursor.
	 */
	if (data->match_fn && (!data->match_fn(cursor->path, 0) || !data->match_fn(cursor->path, 1))) {
		SYS_LOG_ERR("cursor %s is filtered out\n", cursor->path);
		return -EINVAL;
	}

	return 0;
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
	res = fs_opendir(data->dirs[0], data->full_path);
	if (res) {
		SYS_LOG_ERR("fs_opendir %s failed (res=%d)\n", data->full_path, res);
		return res;
	}

	data->level = 0;
	return 0;
}

#ifdef SAVE_CURSOR_SERACHING_LIST

static int file_iterator_set_cursor(struct iterator *iter, const void *iter_cursor)
{
	struct file_iterator_data *data = iter->data;
	const struct file_iterator_cursor *cursor = iter_cursor;
	const char *path;
	int res = -ENOENT;

	res = _validate_cursor(data, cursor);
	if (res)
		return res;

	res = _back_to_topdir(data);
	if (res)
		return res;

	res = -ENOENT;

	/* search cursor */
	do {
		path = next(iter);
		if (!path)
			break;

		if (!strcmp(path, cursor->path)) {
			data->has_query_next = 1;
			res = 0;
			break;
		}
	} while (1);

	if (res) {
		SYS_LOG_WRN("set cursor failed (res=%d), try to travel from topdir\n", res);

		if (!_back_to_topdir(data))
			data->has_query_next = 0;
	}

	return res;
}

#else /* SAVE_CURSOR_SERACHING_LIST */

static int file_iterator_set_cursor(struct iterator *iter, const void *iter_cursor)
{
	struct file_iterator_data *data = iter->data;
	const struct file_iterator_cursor *cursor = iter_cursor;
	const char *path;
	int res = -ENOENT;

	res = _validate_cursor(data, cursor);
	if (res)
		return res;

	res = _back_to_topdir(data);
	if (res)
		return res;

	/* search cursor */
	path = cursor->path + data->dname_len[0];
	while (*path != 0) {
		const char *s_end;
		uint16_t len;

		/* strip heading seperator */
		while (*path == '/' || *path == '\\') path++;

		/* locate the end of subdir or file */
		s_end = path;
		while (*s_end != '/' && *s_end != '\\' && *s_end != 0) s_end++;

		/* get the full path of subdir or file */
		len = s_end - path;
		/* don't forget the dir seperator */
		if (data->full_len + len >= FULL_PATH_LEN - 1) {
			SYS_LOG_WRN("too long path, exceed %u bytes (%s)\n",
				data->full_len + len - FULL_PATH_LEN, data->dirent->name);
			res = -EINVAL;
			break;
		}

		memcpy(&data->full_path[data->full_len], path, len);
		data->full_path[data->full_len + len] = 0;

		path = s_end;

		/* find the matching subdir or file */
		do {
			res = fs_readdir(data->dirs[data->level], data->dirent);
			if (res || data->dirent->name[0] == 0) {
				res = -ENOENT;
				goto out_exit;
			}

			if (strcmp(&data->full_path[data->full_len], data->dirent->name))
				continue;

			if (data->dirent->type == FS_DIR_ENTRY_FILE) {
				data->fname_len = len;
				data->full_len += len;
				res = 0;
				goto out_exit;
			} else {
	 			/* down one level to find */
				if (data->level + 1 >= data->max_level) {
					SYS_LOG_WRN("exceed max level (%u)\n", data->max_level);
					res = -ENOMEM;
					goto out_exit;
				}

				res = fs_opendir(data->dirs[data->level + 1], data->full_path);
				if (res) {
					SYS_LOG_ERR("fs_opendir %s failed\n", data->full_path);
					goto out_exit;
				}

				data->full_path[data->full_len + len] = '/';
				data->full_path[data->full_len + len + 1] = 0;
				data->full_len += len + 1;
				data->dname_len[++data->level] = len + 1;
				break;
			}
		} while (1);
	}

out_exit:
	if (res) {
		SYS_LOG_WRN("set cursor failed (res=%d), try to travel from topdir\n", res);

		if (!_back_to_topdir(data))
			data->has_query_next = 0;
	} else {
		data->has_query_next = 1;
		data->cursor.path = data->full_path;
		iter->cursor = &data->cursor;

		/* save history */
		save_next_to_filist(data);
	}

	return res;
}

#endif /* SAVE_CURSOR_SERACHING_LIST */

static const struct iterator_ops file_iterator_ops = {
	.init = file_iterator_init,
	.destroy = file_iterator_destroy,
	.has_next = file_iterator_has_next,
	.next = file_iterator_next,
	.has_prev = file_iterator_has_prev,
	.prev = file_iterator_prev,
	.set_cursor = file_iterator_set_cursor,
};

struct iterator *file_iterator_create(file_iterator_param_t *param)
{
	return iterator_create((struct iterator_ops *)&file_iterator_ops,param);
}
