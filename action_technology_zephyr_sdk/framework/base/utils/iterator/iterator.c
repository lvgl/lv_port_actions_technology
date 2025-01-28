#include <mem_manager.h>
#include <iterator/iterator.h>
#include <errno.h>
#include <os_common_api.h>

struct iterator *iterator_create(struct iterator_ops *ops, const void *param)
{	struct iterator *iter = mem_malloc(sizeof(struct iterator));

	if (!iter) {
		SYS_LOG_ERR("iter mem malloc fail \n");
		goto out_exit;
	}

	iter->ops = ops;
	if (iter->ops->init) {
		int res = iter->ops->init(iter, param);
		if (res) {
			SYS_LOG_ERR("iterator initialize failed (res=%d)\n", res);
			mem_free(iter);
			return NULL;
		}
	}

out_exit:
	SYS_LOG_INF("iterator create %s\n",iter ? "successfully" : "failed");
	return iter;
}

int iterator_destroy(struct iterator *iter)
{
	int res = 0;

	if (!iter)
		return -EINVAL;

	if (iter->ops->destroy)
		res = iter->ops->destroy(iter);

	mem_free(iter);
	return res;
}

int iterator_has_next(struct iterator *iter)
{
	int res = 0;

	if (iter && iter->ops->has_next)
		res = iter->ops->has_next(iter);

	return res;
}

const void *iterator_next(struct iterator *iter, bool force_switch, uint16_t *track_no)
{
	const void *res = NULL;

	if (iter)
		res = iter->ops->next(iter, force_switch, track_no);

	return res;
}

int iterator_has_prev(struct iterator *iter)
{
	int res = 0;

	if (iter && iter->ops->has_prev)
		res = iter->ops->has_prev(iter);

	return res;
}

const void *iterator_prev(struct iterator *iter, uint16_t *track_no)
{
	const void *res = NULL;

	if (iter && iter->ops->prev)
		res = iter->ops->prev(iter, track_no);

	return res;
}

int iterator_set_cursor(struct iterator *iter, const void *cursor)
{
	if (iter && iter->ops->set_cursor)
		return iter->ops->set_cursor(iter, cursor);

	return -ENOSYS;
}

int iterator_set_mode(struct iterator *iter, uint8_t mode)
{
	if (iter && iter->ops->set_mode)
		return iter->ops->set_mode(iter, mode);

	return -ENOSYS;
}

const void *iterator_next_folder(struct iterator *iter, uint16_t *track_no)
{
	const void *res = NULL;

	if (iter)
		res = iter->ops->next_folder(iter, track_no);

	return res;
}

const void *iterator_prev_folder(struct iterator *iter, uint16_t *track_no)
{
	const void *res = NULL;

	if (iter)
		res = iter->ops->prev_folder(iter, track_no);

	return res;
}

const void *iterator_set_track_no(struct iterator *iter, uint16_t track_no)
{
	const void *res = NULL;

	if (iter)
		res = iter->ops->set_track_no(iter, track_no);

	return res;
}

int iterator_get_plist_info(struct iterator *iter, void *param)
{
	if (iter && iter->ops->get_plist_info)
		return iter->ops->get_plist_info(iter, param);

	return -ENOSYS;
}
