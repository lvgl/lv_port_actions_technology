/*******************************************************************************
 * @file    rbuf_msg_sc.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-10-15
 * @brief   message for sensorhub
*******************************************************************************/

#ifndef _RBUF_MSG_SC_H
#define _RBUF_MSG_SC_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <rbuf/rbuf_msg.h>
#include <drivers/i2cmt.h>
#include <drivers/spimt.h>

/******************************************************************************/
//message define
/******************************************************************************/
typedef enum sc_msg {
	MSG_SC_START = MSG_SC_BASE,
	
	// sc init msg
	MSG_SC_INIT,    // ACK=[major+minor]
	
	// log init
	MSG_SC_LOG_ON, 	// REQ=[size], ACK=[rbuf]
	MSG_SC_LOG_OFF, // REQ=[rbuf], ACK=[ret]
	
	// spi normal msg
	MSG_SC_SPI_NML,  // REQ=[id+op+buf+len], ACK=[id+ret]
	
	// i2c normal msg
	MSG_SC_I2C_NML,  // REQ=[id+op+i2c_xfer], ACK=[id+ret]
	
	// spi task msg
	MSG_SC_SPI_TASK_START, // REQ=[id+task_id+attr], ACK=[id+task_id+irq]
	MSG_SC_SPI_TASK_STOP, // REQ=[id+task_id]
	
	// i2c task msg
	MSG_SC_I2C_TASK_START, // REQ=[id+task_id+attr], ACK=[id+task_id+irq]
	MSG_SC_I2C_TASK_STOP, // REQ=[id+task_id]
	
	MSG_SC_END,

} sc_msg_e;

/******************************************************************************/
//constants
/******************************************************************************/
/* MAIN <--> SENSOR Address Mapping */
#define MAP_SC_FR_CPU(x)		((unsigned int)(x) - INTER_RAM_ADDR + 0x00188000)
#define MAP_SC_TO_CPU(x)		((unsigned int)(x) - 0x00188000 + INTER_RAM_ADDR)

/******************************************************************************/
//typedef
/******************************************************************************/

/*  Message data for sensorhub */
typedef struct sc_spi_msg {
	unsigned short type;    // Message type
	unsigned short flag;    // Message flag
	unsigned short id;      // Bus id
	unsigned short op;      // (0-wr,1-rd,2-cs)
	unsigned char* buf;     // Data buffer
	unsigned short len;     // Data length
} sc_spi_msg_t;

typedef struct sc_i2c_msg {
	unsigned short type;    // Message type
	unsigned short flag;    // Message flag
	unsigned short id;      // Bus id
	unsigned short op;      // (0-wr,1-rd,2-probe)
	i2c_xfer_t xfer;        // I2C transfer
} sc_i2c_msg_t;

typedef struct sc_spi_task_msg {
	unsigned short type;        // Message type
	unsigned short flag;        // Message flag
	unsigned short id;          // Bus id
	unsigned short task_id;     // Task id
	spi_task_t task_attr;       // Task attribute
} sc_spi_task_msg_t;

typedef struct sc_i2c_task_msg {
	unsigned short type;        // Message type
	unsigned short flag;        // Message flag
	unsigned short id;          // Bus id
	unsigned short task_id;     // Task id
	i2c_task_t task_attr;       // Task attribute
} sc_i2c_task_msg_t;

typedef struct sc_task_irq_msg {
	unsigned short type;        // Message type
	unsigned short flag;        // Message flag
	unsigned short id;          // Bus id
	unsigned short task_id;     // Task id
	unsigned short irq_type;    // Irq type
} sc_task_irq_msg_t;

/******************************************************************************/
//macros
/******************************************************************************/

/******************************************************************************/
//functions
/******************************************************************************/

#endif  /* _RBUF_MSG_SC_H */

