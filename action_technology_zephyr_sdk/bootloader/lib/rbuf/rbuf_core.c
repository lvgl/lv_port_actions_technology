/*******************************************************************************
 * @file    rbuf_core.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-10-15
 * @brief   ring buffer for interaction RAM
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <string.h>
#include <rbuf/rbuf_core.h>

/******************************************************************************/
//constants
/******************************************************************************/

/******************************************************************************/
//typedef
/******************************************************************************/

/******************************************************************************/
//variables
/******************************************************************************/

/******************************************************************************/
//functions
/******************************************************************************/
static unsigned int wrap(unsigned int val, unsigned int max)
{
	return val >= max ? (val - max) : val;
}

static unsigned int _get_space(unsigned int size, unsigned int head, unsigned int tail)
{
	if (tail < head) {
		return head - tail - 1;
	} else {
		return (size - tail) + head - 1;
	}
}

static unsigned int _get_rbuf_space(rbuf_t *buf)
{
	return _get_space(buf->size, buf->head, buf->tail);
}

static unsigned int _get_size(rbuf_t *buf, unsigned int size)
{
	return buf->hlen ? ROUND_UP(size, 4) + buf->hlen : size;
}

rbuf_t *rbuf_init_buf(char *buf, unsigned int size, unsigned int mode)
{
	rbuf_t *rbuf = (rbuf_t*)buf;
	
	rbuf->head = rbuf->tmp_head = 0;
	rbuf->tail = rbuf->tmp_tail = 0;
	rbuf->buf_off = RBUF_TO_OF(buf) + sizeof(rbuf_t);
	rbuf->size = size - sizeof(rbuf_t);
	if (mode == RBUF_MSG) {
		rbuf->hlen = sizeof(unsigned int);
	} else {
		rbuf->hlen = 0;
	}
	rbuf->next = 0;
	
	return rbuf;
}

unsigned int rbuf_get_space(rbuf_t *buf)
{
	unsigned int space = _get_rbuf_space(buf);
	return  (space < buf->hlen) ? 0 : (space - buf->hlen);
}

void* rbuf_put_claim(rbuf_t *buf, unsigned int size, unsigned int *psz)
{
	unsigned int space, trail_size, allocated;
	unsigned int *pdata;
	
	/* Available size. */
	size = _get_size(buf, size);
	space = _get_space(buf->size, buf->head, buf->tmp_tail);
	trail_size = buf->size - buf->tmp_tail;

	/* Data pointer */
	pdata = (unsigned int*)RBUF_FR_OF(buf->buf_off + buf->tmp_tail);
	
	if (buf->hlen > 0) {
		/* Limit requested size to available size. */
		if (space < size) {
			return NULL;
		}
		
		/* Limit requested size to be continued */
		if (trail_size < size) {
			if ((space - trail_size) < size) {
				return NULL;
			}
			/* Put Null Msg and Reset tail */
			*pdata = 0;
			buf->tail = buf->tmp_tail = 0;
			
			/* Next Msg */
			pdata = (unsigned int*)RBUF_FR_OF(buf->buf_off);
		}
		
		/* Update data and size */
		allocated = size - buf->hlen;
		*pdata = allocated;
		pdata ++;
	} else {
		/* Limit requested size to available size. */
		size = MIN(size, space);

		/* Limit allocated size to trail size. */
		allocated = MIN(trail_size, size);
		if (allocated == 0) {
			return NULL;
		}
	}
	
	if (psz) {
		*psz = allocated;
	}
	
	buf->tmp_tail = wrap(buf->tmp_tail + allocated + buf->hlen, buf->size);
	
	return pdata;
}

int rbuf_put_finish(rbuf_t *buf, unsigned int size)
{
	size = _get_size(buf, size);
	if (size > _get_rbuf_space(buf)) {
		return -1;
	}

	buf->tail = wrap(buf->tail + size, buf->size);
	buf->tmp_tail = buf->tail;

	return 0;
}

void* rbuf_get_claim(rbuf_t *buf, unsigned int size, unsigned int *psz)
{
	unsigned int space, granted_size, trail_size;
	unsigned int *pdata;

	/* Check space */
	space = (buf->size - 1) - _get_space(buf->size, buf->tmp_head, buf->tail);
	if (space <= 0) {
		return NULL;
	}
	
	/* Data Pointer */
	pdata = (unsigned int*)RBUF_FR_OF(buf->buf_off + buf->tmp_head);
	
	/* Msg process */
	if (buf->hlen > 0) {
		/* Skip null data and reset head */
		if (*pdata == 0) {
			buf->head = buf->tmp_head = 0;
			/* Check empty */
			if (buf->head == buf->tail) {
				return NULL;
			}
			pdata = (unsigned int*)RBUF_FR_OF(buf->buf_off);
		}
		
		/* Update data and size */
		granted_size = *pdata;
		pdata ++;
	} else {
		/* Limit requested size to available size. */
		granted_size = MIN(size, space);
		
		/* Limit allocated size to trail size. */
		trail_size = buf->size - buf->tmp_head;
		granted_size = MIN(trail_size, granted_size);
	}
	
	if (psz) {
		*psz = granted_size;
	}
	
	buf->tmp_head =	wrap(buf->tmp_head + granted_size + buf->hlen, buf->size);
	
	return pdata;
}

int rbuf_get_finish(rbuf_t *buf, unsigned int size)
{
	unsigned int allocated;

	size = _get_size(buf, size);
	allocated = (buf->size - 1) - _get_rbuf_space(buf);
	if (size > allocated) {
		return -1;
	}

	buf->head = wrap(buf->head + size, buf->size);
	buf->tmp_head = buf->head;

	return 0;
}

unsigned int rbuf_get_hdl(rbuf_t *buf, unsigned int size, rbuf_hdl hdl, void *ctx)
{
	void *pdata;

	/* read data */
	pdata = rbuf_get_claim(buf, size, &size);
	if (pdata != NULL) {
		/* handle data */
		size = (*hdl)(ctx, pdata, size);
	}
	
	/* finish data */
	rbuf_get_finish(buf, size);
	
	return size;
}
