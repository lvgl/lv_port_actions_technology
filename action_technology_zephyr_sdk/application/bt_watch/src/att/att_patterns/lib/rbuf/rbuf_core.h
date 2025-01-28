/*******************************************************************************
 * @file    rbuf_core.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-10-15
 * @brief   ring buffer for interaction RAM
*******************************************************************************/

#ifndef _RBUF_CORE_H
#define _RBUF_CORE_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <rbuf/rbuf_conf.h>

/******************************************************************************/
//constants
/******************************************************************************/
// RBuf Type
#define RBUF_RAW					(0 << 0)
#define RBUF_MSG					(1 << 0)

/******************************************************************************/
//typedef
/******************************************************************************/

/* RBuf Data */
typedef struct rbuf {
	unsigned short head;
	unsigned short tmp_head;
	unsigned short tail;
	unsigned short tmp_tail;
	unsigned short buf_off;
	unsigned short size;
	unsigned short hlen;
	unsigned short next;
} rbuf_t;

/* RBuf Handler */
typedef int (*rbuf_hdl)(void *ctx, void *pdata, unsigned int size);

/******************************************************************************/
//macros
/******************************************************************************/
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef ROUND_UP
/* round "x" up to next multiple of "align" (which must be a power of 2) */
#define ROUND_UP(x, a) \
			(((unsigned int)(x) + ((unsigned int)(a) - 1)) & ~((unsigned int)(a) - 1))
#endif

/* Ring Buffer from/to Offset */
#define RBUF_NULL					(void*)(RBUF_BASE)
#define RBUF_FR_OF(x)			(rbuf_t*)(RBUF_BASE + x)
#define RBUF_TO_OF(x)			((unsigned int)x - RBUF_BASE)
#define RBUF_FR_BUF(x)		(rbuf_t*)(x - sizeof(rbuf_t))
#define RBUF_TO_BUF(x)		((char*)x + sizeof(rbuf_t))

/******************************************************************************/
//functions
/******************************************************************************/
rbuf_t *rbuf_init_buf(char *buf, unsigned int size, unsigned int mode);

unsigned int rbuf_get_space(rbuf_t *buf);

void* rbuf_put_claim(rbuf_t *buf, unsigned int size, unsigned int *psz);
int rbuf_put_finish(rbuf_t *buf, unsigned int size);

void* rbuf_get_claim(rbuf_t *buf, unsigned int size, unsigned int *psz);
int rbuf_get_finish(rbuf_t *buf, unsigned int size);

unsigned int rbuf_get_hdl(rbuf_t *buf, unsigned int size, rbuf_hdl hdl, void *ctx);

#define rbuf_is_empty(x)		((x)->head == (x)->tail)
#define rbuf_is_pending(x)		((x) && !rbuf_is_empty((x)))

#endif  /* _RBUF_CORE_H */

