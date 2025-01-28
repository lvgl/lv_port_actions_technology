/*****************************************************************************/
/*                                                                           */
/*    Copyright 2024 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.   */
/*                                                                           */
/*****************************************************************************/

/**
 * @file aic_srv_at.h
 *
 */

#ifndef __AIC_SRV_AT_H_
#define __AIC_SRV_AT_H_

#ifdef __cplusplus
extern "C" {
#endif
/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stdbool.h>

/**
* aic_srv_at_read - read buffer from AT channel
* p_buffer [OUT]: the received buffer
* size [IN]: the size of received buffer
* Return the length of received buffer
*/
uint32_t aic_srv_at_read(void *p_buffer, uint32_t size);

/**
* aic_srv_at_write - write buffer to AT channel
* p_buffer [IN]: the buffer will be sent
* Return the length of buffer
*/
uint32_t aic_srv_at_write(const void *p_buffer);

/**
* aic_srv_at_init - initial AT module
* Return the result of initial
*/
int32_t aic_srv_at_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __AIC_SRV_AT_H__ */

