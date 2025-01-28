/******************************************************************************/
/*                                                                            */
/*    Copyright 2024 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 * @file aic_net.c
 *
 */
/*********************
 *      INCLUDES
 *********************/
#include "aic_srv_cfg.h"

void aic_net_init(void)
{
#if AIC_CAT1_ENABLE_LWIP_NETWORK_SOLUTION
    extern int lwip_pdp_init(void);
    lwip_pdp_init();
#endif
#if AIC_CAT1_ENABLE_AT_NETWORK_SOLUTION
    extern int aic_at_network_init(void);
    aic_at_network_init();
#endif
#if AIC_CAT1_ENABLE_AT_SOCKET_SOLUTION
    extern int aic_at_socket_init();
    aic_at_socket_init();
#endif
}

