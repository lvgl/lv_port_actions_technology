/******************************************************************************/
/*                                                                            */
/*    Copyright 2024 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

#ifndef __AIC_NETWORK_SPEED_H__
#define __AIC_NETWORK_SPEED_H__

typedef void (*aic_network_speed_callback)(int errcode,int dlspeed);
int aic_network_speed_download_test(aic_network_speed_callback callback);
int aic_network_speed_upload_test(aic_network_speed_callback callback);

#define SPEED_NO_ERROR 0
#define CREATE_SOCKET_ERROR 1001
#endif /* __AIC_NETWORK_SPEED_H__ */

