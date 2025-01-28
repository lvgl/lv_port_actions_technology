#ifndef __FILE_ITERATOR_H__
#define __FILE_ITERATOR_H__

#include <iterator/iterator.h>

/** ITERATOR_TYPE_FILE cursor structure */
typedef struct file_iterator_cursor {
	const char *path;
} file_iterator_cursor_t;

/** ITERATOR_TYPE_FILE param structure */
typedef struct file_iterator_param {
	int max_level; /* maximum levels to search */
	const char *topdir;
	const struct file_iterator_cursor *cursor;

	/* match callback to filter path, return 1 if matched */
	int (*match_fn)(const char *path, int is_dir);
} file_iterator_param_t;

/**
 * @brief create file iterator
 *
 * This routine creates file iterator, and return the address of iterator.
 *
 * @param param parameter of iterator
 *
 * @return address of the created iterator
 */
struct iterator *file_iterator_create(file_iterator_param_t *param);

#endif /* __FILE_ITERATOR_H__ */
