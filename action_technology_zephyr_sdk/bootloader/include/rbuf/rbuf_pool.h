/*******************************************************************************
 * @file    rbuf_pool.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-10-15
 * @brief   ring buffer for interaction RAM
*******************************************************************************/

#ifndef _RBUF_POOL_H
#define _RBUF_POOL_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <rbuf/rbuf_core.h>

/******************************************************************************/
//constants
/******************************************************************************/
#define RBUF_POOL_DBG					(0)

/******************************************************************************/
//typedef
/******************************************************************************/

// Pool Node
typedef struct rbuf_node_s {
	unsigned short blk_size;
	unsigned short blk_list;
	unsigned short next;
	unsigned short flag;
} rbuf_node_t;

// Pool Header
typedef struct rbuf_pool_s {
	unsigned short buf_off;
	unsigned short buf_size;
	rbuf_node_t root;
} rbuf_pool_t;

/******************************************************************************/
//variables
/******************************************************************************/

/******************************************************************************/
//functions
/******************************************************************************/
rbuf_pool_t *rbuf_pool_init(char *buf, unsigned int size);

rbuf_t *rbuf_pool_alloc(rbuf_pool_t *pool, unsigned int size, unsigned int mode);

int rbuf_pool_free(rbuf_t *buf);

#if RBUF_POOL_DBG
	void rbuf_pool_dump(rbuf_pool_t *pool);
#else
	#define rbuf_pool_dump(x)
#endif

#endif  /* _RBUF_POOL_H */

