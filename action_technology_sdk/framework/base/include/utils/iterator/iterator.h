#ifndef __ITERATOR_H__
#define __ITERATOR_H__

#include <stdbool.h>

/** iterator types */
typedef enum iterator_type {
	ITERATOR_TYPE_FILE = 0,

	ITERATOR_NUM_TYPES,
} iterator_type_t;

struct iterator;

/** iterator ops structure */
typedef struct iterator_ops {
	/* initialize the iterator */
	int (*init)(struct iterator *iter, const void *param);
	/* destroy the iterator and release all resource */
	int (*destroy)(struct iterator *iter);

	/* query whether the forward iteration has more elements */
	int (*has_next)(struct iterator *iter);
	/* iterate forward and returns the element in the iteration */
	const void *(*next)(struct iterator *iter, bool force_switch, uint16_t *track_no);

	/* (optional) query whether the backward iteration has more elements */
	int (*has_prev)(struct iterator *iter);
	/* (optional) iterate backward and returns the element in the iteration */
	const void *(*prev)(struct iterator *iter, uint16_t *track_no);

	/* (optional) set current cursor in the iteration */
	int (*set_cursor)(struct iterator *iter, const void *cursor);

	/* (optional) set current mode in the iteration */
	int (*set_mode)(struct iterator *iter, uint8_t mode);

	/* (optional) iterate forward next folder and return the element in the iteration */
	const void *(*next_folder)(struct iterator *iter, uint16_t *track_no);

	/* (optional) iterate backward prev folder and return the element in the iteration */
	const void *(*prev_folder)(struct iterator *iter, uint16_t *track_no);

	/* (optional) iterate set to track no and return the element in the iteration */
	const void *(*set_track_no)(struct iterator *iter, uint16_t track_no);

	/* (optional) get plist info in the iteration */
	int (*get_plist_info)(struct iterator *iter, void *param);
} iterator_ops_t;

/** iterator structure */
typedef struct iterator {
	int type;
	void *data;
	const void *cursor;
	const struct iterator_ops *ops;
} iterator_t;


/**
 * @brief create iterator
 *
 * This routine provides create iterator, and return the address of iterator.
 **@param ops sub iterator operations
 * @param type type of iterator
 * @param param parameter of iterator
 *
 * @return address of the created iterator
 */
struct iterator *iterator_create(struct iterator_ops *ops, const void *param);

/**
 * @brief destroy iterator
 *
 * This routine provides destroy iterator, and destroy all the resourse.
 *
 * @param iter address of the iterator
 *
 * @return 0 if succeed, others failed
 */
int iterator_destroy(struct iterator *iter);

/**
 * @brief query whether the forward iteration has more elements.
 *
 * This routine provides query whether the forward iteration has more elements.
 *
 * @param iter address of the iterator
 *
 * @return 1 if has more elements, 0 if no more elements.
 */
int iterator_has_next(struct iterator *iter);

/**
 * @brief get the next element in the iteration.
 *
 * This routine provides iterate forward, and get the element.
 *
 * @param iter address of the iterator
 *
 * @return the element
 */
const void *iterator_next(struct iterator *iter, bool force_switch, uint16_t *track_no);

/**
 * @brief query whether the backward iteration has more elements.
 *
 * This routine provides query whether the backward iteration has more elements.
 *
 * @param iter address of the iterator
 *
 * @param force_switch force to get next elements
 *
 * @return 1 if has more elements, 0 if no more elements.
 */
int iterator_has_prev(struct iterator *iter);

/**
 * @brief get the previous element in the iteration.
 *
 * This routine provides iterate backward, and get the element.
 *
 * @param iter address of the iterator
 *
 * @return the element
 */
const void *iterator_prev(struct iterator *iter, uint16_t *track_no);

/**
 * @brief get current cursor in the iteration.
 *
 * This routine provides get current cursor in the iteration.
 *
 * @param iter address of the iterator
 *
 * @return the cursor
 */
static inline const void *iterator_get_cursor(struct iterator *iter)
{
	return iter->cursor;
}

/**
 * @brief set current cursor in the iteration.
 *
 * This routine provides set current cursor in the iteration.
 *
 * @param iter address of the iterator
 *
 * @return 0 if succeed, others failed
 */
int iterator_set_cursor(struct iterator *iter, const void *cursor);

/**
 * @brief set current mode in the iteration.
 *
 * This routine provides set current mode in the iteration.
 *
 * @param iter address of the iterator
 *
 * @param mode mode of the iterator
 *
 * @return 0 if succeed, others failed
 */
int iterator_set_mode(struct iterator *iter, uint8_t mode);

/**
 * @brief get the next folder element in the iteration.
 *
 * This routine provides iterate forward, and get the element.
 *
 * @param iter address of the iterator
 *
 * @return the element
 */
const void *iterator_next_folder(struct iterator *iter, uint16_t *track_no);

/**
 * @brief get the previous folder element in the iteration.
 *
 * This routine provides iterate backward, and get the element.
 *
 * @param iter address of the iterator
 *
 * @return the element
 */
const void *iterator_prev_folder(struct iterator *iter, uint16_t *track_no);

/**
 * @brief set current track_no in the iteration.
 *
 * This routine provides set current track_no in the iteration.
 *
 * @param iter address of the iterator
 *
 * @param track_no current track_no
 *
 * @return 0 if succeed, others failed
 */
const void *iterator_set_track_no(struct iterator *iter, uint16_t track_no);

/**
 * @brief get plist info from the iteration.
 *
 * This routine provides get plist info from the iteration.
 *
 * @param iter address of the iterator
 *
 * @param param address of the info
 *
 * @return 0 if succeed, others failed
 */
int iterator_get_plist_info(struct iterator *iter, void *param);
#endif /* __ITERATOR_H__ */
