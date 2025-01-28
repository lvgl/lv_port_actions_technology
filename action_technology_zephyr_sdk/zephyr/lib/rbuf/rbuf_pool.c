/*******************************************************************************
 * @file    rbuf_pool.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-10-15
 * @brief   ring buffer pool for interaction RAM
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <string.h>
#include <stdio.h>
#include <rbuf/rbuf_pool.h>

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
static rbuf_node_t *_pool_alloc_node(rbuf_pool_t *pool, unsigned int size)
{
	rbuf_node_t *node = RBUF_NULL;
	
	if (pool->buf_size >= sizeof(rbuf_node_t)) {
		/* alloc from free buffer */
		node = (rbuf_node_t*)RBUF_FR_OF(pool->buf_off);
		node->blk_list = 0;
		node->blk_size = size;
		node->next = 0;
		/* update free buffer */
		pool->buf_off += sizeof(rbuf_node_t);
		pool->buf_size -= sizeof(rbuf_node_t);
	}
	
	return node;
}

static rbuf_node_t *_pool_get_node(rbuf_pool_t *pool, unsigned int size)
{
	rbuf_node_t *node = &pool->root;
	rbuf_node_t *prev = NULL;
	
	/* node search */
	while ((node != RBUF_NULL) && (node->blk_size < size)) {
		prev = node;
		node = (rbuf_node_t*)RBUF_FR_OF(node->next);
	}
	
	/* add node to list */
	if ((node == RBUF_NULL) || (node->blk_size > size)) {
		node = _pool_alloc_node(pool, size);
		if ((node != RBUF_NULL) && (prev != NULL)) {
			node->next = prev->next;
			prev->next = RBUF_TO_OF(node);
		}
	}
	
	return node;
}

rbuf_pool_t *rbuf_pool_init(char *buf, unsigned int size)
{
	rbuf_pool_t *pool = (rbuf_pool_t*)buf;
	
	pool->buf_off = RBUF_TO_OF(buf) + sizeof(rbuf_pool_t);
	pool->buf_size = size - sizeof(rbuf_pool_t);
	pool->root.blk_size = 0;
	pool->root.blk_list = 0;
	pool->root.next = 0;
	
	return pool;
}

rbuf_t *rbuf_pool_alloc(rbuf_pool_t *pool, unsigned int size, unsigned int mode)
{
	rbuf_node_t *node;
	rbuf_t *buf = NULL;
	unsigned int buf_size;
	
	/* get node */
	size = ROUND_UP(size, 32);
	node = _pool_get_node(pool, size);
	if (node == RBUF_NULL) {
		return NULL;
	}
	
	/* alloc rbuf */
	while (node != RBUF_NULL) {
		buf_size = sizeof(rbuf_t) + node->blk_size;
		if (node->blk_list > 0) {
			/* alloc from free blk list */
			buf = RBUF_FR_OF(node->blk_list);
			node->blk_list = buf->next;
			break;
		}
		if (pool->buf_size >= buf_size) {
			/* alloc from free buffer */
			buf = RBUF_FR_OF(pool->buf_off);
			pool->buf_off += buf_size;
			pool->buf_size -= buf_size;
			break;
		}
		node = (rbuf_node_t*)RBUF_FR_OF(node->next);
	}
	
	/* init rbuf */
	if (buf != NULL) {
		rbuf_init_buf((char*)buf, buf_size, mode);
		buf->next = RBUF_TO_OF(node);
	}
	
	return buf;
}

int rbuf_pool_free(rbuf_t *buf)
{
	rbuf_node_t *node;
	
	if (buf->next != 0) {
		node = (rbuf_node_t*)RBUF_FR_OF(buf->next);
		/* free to free blk list */
		buf->next = node->blk_list;
		node->blk_list = RBUF_TO_OF(buf);
	}
	
	return 0;
}

#if RBUF_POOL_DBG
void rbuf_pool_dump(rbuf_pool_t *pool)
{
	rbuf_node_t *node = &pool->root;
	rbuf_t *buf;
	
	/* dump free */
	printf("free: [0x%04x] [%d]\r\n", (unsigned int)pool->buf_off, pool->buf_size);
	printf("\r\n");
	
	while (node != RBUF_NULL) {
		/* dump node */
		printf("node: [0x%04x] [%d]\r\n", RBUF_TO_OF(node), node->blk_size);
		/* dump list */
		buf = RBUF_FR_OF(node->blk_list);
		while (buf != RBUF_NULL) {
			printf("  -> rbuf: [0x%04x] [%d] [%d]\r\n", RBUF_TO_OF(buf), buf->size, buf->hlen);
			buf = RBUF_FR_OF(buf->next);
		}
		node = (rbuf_node_t*)RBUF_FR_OF(node->next);
	}
	
	printf("\r\n");
}
#endif

