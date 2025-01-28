/*******************************************************************************
 * @file    rbuf_msg.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-10-15
 * @brief   message for interaction RAM
*******************************************************************************/

#ifndef _RBUF_MSG_H
#define _RBUF_MSG_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <rbuf/rbuf_mem.h>
#include <rbuf/rbuf_core.h>
#include <rbuf/rbuf_pool.h>

/******************************************************************************/
//message define
/******************************************************************************/

typedef enum rb_msg {
	/* Message base for system */
	MSG_SYS_BASE = 0x100,
	
	/* Message base for main cpu */
	MSG_MAIN_BASE = 0x200,
	
	/* Message base for dsp */
	MSG_DSP_BASE = 0x300,
	
	/* Message base for bt cpu */
	MSG_BT_BASE = 0x400,
	MSG_BT_INIT,
	MSG_BT_HCI_BUF,
	MSG_BT_HCI_OK,
	MSG_BT_LOG_ON,
	MSG_BT_LOG_OFF,
	MSG_BT_MDM_RF_DEBUG,
	MSG_BT_GEN_P192_PK,
	MSG_BT_GEN_P192_DHKEY,
	MSG_BT_GEN_P256_PK,
	MSG_BT_GEN_P256_DHKEY,
	MSG_BT_TWS_BUF,
	MSG_BT_READ_BB_REG,
	MSG_BT_WRITE_BB_REG,
	MSG_BT_READ_RF_REG,
	MSG_BT_WRITE_RF_REG,
	
	/* Message base for sensor cpu */
	MSG_SC_BASE = 0x500,
} rbuf_msg_e;

/******************************************************************************/
//constants
/******************************************************************************/

// Rbuf Size for Message
#define RB_MSG_SIZE					(256)

/******************************************************************************/
//typedef
/******************************************************************************/

/* Message Data */
typedef struct rbuf_msg {
	/* Message Type */
	unsigned short type;
	
	/* Message Flag */
	unsigned short flag;
	
	/* Message Data Union */
	union rbuf_msg_dat {
		/* Message 32bit Data */
		unsigned int w[4];
		
		/* Message 16bit Data */
		unsigned short h[8];
		
		/* Message 8bit Data */
		unsigned char b[16];
	} data;
} rbuf_msg_t;

/******************************************************************************/
//macros
/******************************************************************************/
#define RB_MSG(f,t)								RBUF_FR_OF(RB_GET_MSG(f,t))
#define RB_MSG_INIT()							rbuf_pool_init((char*)RB_ST_POOL, RB_SZ_POOL)
#define RB_MSG_ALLOC(sz)					rbuf_pool_alloc(RB_GET_POOL,sz,RBUF_MSG)
#define RB_MSG_FREE(rbuf)					rbuf_pool_free(rbuf)

/******************************************************************************/
//functions
/******************************************************************************/
#define rbuf_msg_create(fr,to,sz)				RB_GET_MSG(fr,to)=RBUF_TO_OF(RB_MSG_ALLOC(sz))
#define rbuf_msg_destroy(fr,to)					rbuf_pool_free(RB_MSG(fr,to));RB_GET_MSG(fr,to)=0

#define rbuf_msg_claim(fr,to,sz)				rbuf_put_claim(RB_MSG(fr,to),sz,0)
#define rbuf_msg_send(fr,to,sz)					rbuf_put_finish(RB_MSG(fr,to),sz)
#define rbuf_msg_recv(fr,to,sz,hdl,ctx)			rbuf_get_hdl(RB_MSG(fr,to),sz,hdl,ctx)
#define rbuf_msg_pending(fr,to)					!rbuf_is_empty(RB_MSG(fr,to))

#endif  /* _RBUF_MSG_H */

